#include <sstream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ColorTextReporter.cpp"

using namespace std;
using namespace ::testing;

class MockRuleBase : public RuleBase
{
public:
    MOCK_METHOD0(apply, void());
    MOCK_CONST_METHOD0(name, const string());
    MOCK_CONST_METHOD0(priority, const int());
};

class ColorTextReporterTest : public ::testing::Test
{
protected:
    virtual void setUp()
    {
        ColorTextReporter reporter;
    }

    ColorTextReporter reporter;
};

TEST_F(ColorTextReporterTest, PropertyTest)
{
    EXPECT_THAT(reporter.name(), StrEq("color"));
}

TEST_F(ColorTextReporterTest, WriteHeader)
{
    ostringstream oss;
    reporter.writeHeader(oss);
    EXPECT_THAT(oss.str(), StrEq("OCLint Report"));
}

TEST_F(ColorTextReporterTest, WriteFooter)
{
    ostringstream oss;
    reporter.writeFooter(oss, "-test");
    EXPECT_THAT(oss.str(), StrEq("[OCLint (http://oclint.org) v-test]"));
}

TEST_F(ColorTextReporterTest, WriteSummary)
{
    Results *restults = Results::getInstance();
    ostringstream oss;
    reporter.writeSummary(oss, *restults);
    EXPECT_THAT(oss.str(), StartsWith("Summary:"));
    EXPECT_THAT(oss.str(), HasSubstr("TotalFiles=0"));
    EXPECT_THAT(oss.str(), HasSubstr("FilesWithViolations=0"));
    EXPECT_THAT(oss.str(), HasSubstr("P1=0"));
    EXPECT_THAT(oss.str(), HasSubstr("P2=0"));
    EXPECT_THAT(oss.str(), HasSubstr("P3=0"));
}

TEST_F(ColorTextReporterTest, WriteViolation)
{
    RuleBase *rule = new MockRuleBase();
    Violation violation(rule, "test path", 1, 2, 3, 4, "test message");
    ostringstream oss;
    reporter.writeViolation(oss, violation);
    EXPECT_THAT(oss.str(), HasSubstr("test path"));
    EXPECT_THAT(oss.str(), HasSubstr("1:2"));
    EXPECT_THAT(oss.str(), HasSubstr("test message"));
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
