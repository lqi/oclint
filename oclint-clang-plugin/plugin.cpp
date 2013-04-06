#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/FileSystem.h>
#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/RecordLayout.h>
#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Driver/OptTable.h>
#include <clang/Driver/Options.h>

#include "oclint/RuleConfiguration.h"
#include "oclint/RuleSet.h"
#include "oclint/Results.h"
#include "oclint/ViolationSet.h"
#include "oclint/Violation.h"
#include "oclint/RuleBase.h"
#include "oclint/Reporter.h"

using namespace std;
using namespace llvm;
using namespace llvm::sys;
using namespace clang;
using namespace clang::driver;

static Reporter *selectedReporter = NULL;

string getExecutablePath(const char *argv)
{
    llvm::SmallString<128> installedPath(argv);
    if (path::filename(installedPath) == installedPath)
    {
        Path intermediatePath = Program::FindProgramByName(path::filename(installedPath.str()));
        if (!intermediatePath.empty())
        {
            installedPath = intermediatePath.str();
        }
    }
    fs::make_absolute(installedPath);
    installedPath = path::parent_path(installedPath);
    return string(installedPath.c_str());
}

int dynamicLoadRules(string ruleDirPath)
{
    DIR *dp = opendir(ruleDirPath.c_str());
    if (dp != NULL)
    {
        struct dirent *dirp;
        while ((dirp = readdir(dp)))
        {
            if (dirp->d_name[0] == '.')
            {
                continue;
            }
            string rulePath = ruleDirPath + "/" + string(dirp->d_name);
            cout << "each rule path: " << rulePath << endl;
            if (dlopen(rulePath.c_str(), RTLD_LAZY) == NULL)
            {
                cerr << dlerror() << endl;
                closedir(dp);
                return 3;
            }
        }
        closedir(dp);
    }
    return 0;
}

int consumeArgRulesPath(const char* executablePath)
{
    cout << "load rule: " << executablePath << endl;
    string exeStrPath = getExecutablePath(executablePath);
    string defaultRulePath = exeStrPath + "/../lib/oclint/rules";
    cout << "full rule path" << defaultRulePath <<endl;
    return dynamicLoadRules(defaultRulePath);
}

int loadReporter(const char* executablePath)
{
    cout << "load reporter: " << executablePath << endl;
    selectedReporter = NULL;
    string exeStrPath = getExecutablePath(executablePath);
    string defaultReportersPath = exeStrPath + "/../lib/oclint/reporters";
    DIR *dp = opendir(defaultReportersPath.c_str());
    if (dp != NULL)
    {
        struct dirent *dirp;
        while ((dirp = readdir(dp)))
        {
            if (dirp->d_name[0] == '.')
            {
                continue;
            }
            string reporterPath = defaultReportersPath + "/" + string(dirp->d_name);
            void *reporterHandle = dlopen(reporterPath.c_str(), RTLD_LAZY);
            if (reporterHandle == NULL)
            {
                cerr << dlerror() << endl;
                closedir(dp);
            }
            Reporter* (*createMethodPointer)();
            createMethodPointer = (Reporter* (*)())dlsym(reporterHandle, "create");
            Reporter* reporter = (Reporter*)createMethodPointer();
            if (reporter->name() == "text")
            {
                selectedReporter = reporter;
                break;
            }
        }
        closedir(dp);
    } // TODO: Remove the duplication pf loading rules and reporters
    return selectedReporter == NULL ? 1 : 0;
}

void consumeRuleConfigurations()
{
    vector<string> argRuleConfiguration;
    // TODO: No rule configuration for now, but to trick the compiler load RuleConfiguration symbols
    for (unsigned i = 0; i < argRuleConfiguration.size(); ++i)
    {
        string configuration = argRuleConfiguration[i];
        int indexOfSeparator = configuration.find_last_of("=");
        string key = configuration.substr(0, indexOfSeparator);
        string value = configuration.substr(indexOfSeparator + 1,
            configuration.size() - indexOfSeparator - 1);
        RuleConfiguration::addConfiguration(key, value);
    }
}

ostream* outStream()
{
    return &cout;
}

Reporter* reporter()
{
    return selectedReporter;
}

class Processor : public ASTConsumer
{
public:
    virtual void HandleTranslationUnit(ASTContext &astContext)
    {
        ViolationSet *violationSet = new ViolationSet();
        RuleCarrier *carrier = new RuleCarrier(&astContext, violationSet);
        for (int index = 0, numRules = RuleSet::numberOfRules(); index < numRules; index++)
        {
            RuleSet::getRuleAtIndex(index)->takeoff(carrier);
        }
        Results *results = Results::getInstance();
        results->add(violationSet);
    }
};

class ProcessorActionFactory : public PluginASTAction
{
protected:
    ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef)
    {
        return new Processor();
    }

    bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string>& args)
    {
        if (args.size() == 0)
        {
            llvm::errs() << "Please specify the oclint lib folder manually by -plugin-arg-oclint <path>\n";
            return false;
        }
        string path = args[0];
        if (consumeArgRulesPath(path.c_str()) || RuleSet::numberOfRules() <= 0)
        {
            return false;
        }
        if (loadReporter(path.c_str()))
        {
            return false;
        }
        consumeRuleConfigurations();
        return true;
    }

    virtual void EndSourceFileAction()
    {
        ostream *out = outStream();
        Results *results = Results::getInstance();
        reporter()->report(results, *out);
    }
};

static FrontendPluginRegistry::Add<ProcessorActionFactory>
X("oclint", "analyze code quality");
