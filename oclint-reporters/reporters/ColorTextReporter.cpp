#include "oclint/Reporter.h"
#include "oclint/RuleBase.h"
#include "oclint/Version.h"
#include "oclint/ViolationSet.h"

class ColorTextReporter : public Reporter
{
public:
    virtual const string name() const
    {
        return "color";
    }

    virtual void report(Results *results, ostream &out)
    {
        writeHeader(out);
        out << endl << endl;
        writeSummary(out, *results);
        out << endl << endl;
        vector<Violation> violationSet = results->allViolations();
        for (int index = 0, numberOfViolations = violationSet.size();
            index < numberOfViolations; index++)
        {
            writeViolation(out, violationSet.at(index));
            out << endl;
        }
        out << endl;
        writeFooter(out, Version::identifier());
        out << endl;
    }

    void writeHeader(ostream &out)
    {
        out << "OCLint Report";
    }

    void writeFooter(ostream &out, string version)
    {
        out << "[OCLint (http://oclint.org) v" << version << "]";
    }

    void writeSummary(ostream &out, Results &results)
    {
        out << "Summary: TotalFiles=" << results.numberOfFiles() << " ";
        out << "FilesWithViolations=" << results.numberOfFilesWithViolations() << " ";
        out << "P1=" << results.numberOfViolationsWithPriority(1) << " ";
        out << "P2=" << results.numberOfViolationsWithPriority(2) << " ";
        out << "P3=" << results.numberOfViolationsWithPriority(3) << " ";
    }

    void writeViolation(ostream &out, Violation &violation)
    {
        const RuleBase *rule = violation.rule;
        switch (rule->priority())
        {
            case 1:
                out << "\e[0;31m";
                break;
            case 2:
                out << "\e[0;33m";
                break;
            case 3:
                out << "\e[0;32m";
                break;
            default:
                out << "\e[0m";
                break;
        }
        out << violation.path << ":" << violation.startLine << ":" << violation.startColumn;
        out << ": " << rule->name() << " P" << rule->priority() << " " << violation.message;
        out << "\e[0m";
    }
};

extern "C" Reporter* create()
{
  return new ColorTextReporter();
}
