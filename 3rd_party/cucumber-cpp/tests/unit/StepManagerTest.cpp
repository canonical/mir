#include <gtest/gtest.h>

#include "../utils/StepManagerTestDouble.hpp"

#include <boost/assign/list_of.hpp>
#include <map>

using namespace std;
using namespace boost::assign;
using namespace cuke::internal;


class StepManagerTest : public ::testing::Test {
public:
    StepManagerTestDouble stepManager;

    StepManagerTest() :
        stepManager() {
    }
protected:
    const static char *a_matcher;
    const static char *another_matcher;
    const static char *no_match;
    const static char *a_third_matcher;
    const map<int, string> no_params;

    int getUniqueMatchIdOrZeroFor(const string &stepMatch) {
        MatchResult::match_results_type resultSet = getResultSetFor(stepMatch);
        if (resultSet.size() != 1) {
            return 0;
        } else {
            return resultSet.begin()->stepInfo->id;
        }
    }

    int countMatches(const string &stepMatch) {
        return getResultSetFor(stepMatch).size();
    }

    bool matchesOnce(const string &stepMatch) {
        return (countMatches(stepMatch) == 1);
    }

    bool matchesAtLeastOnce(const string &stepMatch) {
        return (countMatches(stepMatch) > 0);
    }

    bool extractedParamsAre(const string stepMatch, map<int, string> params) {
        MatchResult::match_results_type resultSet = getResultSetFor(stepMatch);
        if (resultSet.size() != 1) {
            return false; // more than one match
        }
        SingleStepMatch match = resultSet.front();
        if (params.size() != match.submatches.size()) {
            return false;
        }
        SingleStepMatch::submatches_type::const_iterator rsi;
        for (rsi = match.submatches.begin(); rsi != match.submatches.end(); ++rsi) {
            if (params.find(rsi->position) == params.end())
                return false;
            if (rsi->value != params[rsi->position]) {
                return false;
            }
        }
        return true;
    }
private:
    MatchResult::match_results_type getResultSetFor(const string &stepMatch) {
        return stepManager.stepMatches(stepMatch).getResultSet();
    }
    void TearDown() {
        stepManager.clearSteps();
    }
};

const char *StepManagerTest::a_matcher = "a matcher";
const char *StepManagerTest::another_matcher = "another matcher";
const char *StepManagerTest::a_third_matcher = "a third matcher";
const char *StepManagerTest::no_match = "no match";

TEST_F(StepManagerTest, holdsNonConflictingSteps) {
    ASSERT_EQ(0, stepManager.count());
    stepManager.addStepDefinition(a_matcher);
    stepManager.addStepDefinition(another_matcher);
    stepManager.addStepDefinition(a_third_matcher);
    ASSERT_EQ(3, stepManager.count());
}

TEST_F(StepManagerTest, holdsConflictingSteps) {
    ASSERT_EQ(0, stepManager.count());
    stepManager.addStepDefinition(a_matcher);
    stepManager.addStepDefinition(a_matcher);
    stepManager.addStepDefinition(a_matcher);
    ASSERT_EQ(3, stepManager.count());
}

TEST_F(StepManagerTest, matchesStepsWithNonRegExMatchers) {
    EXPECT_FALSE(matchesAtLeastOnce(no_match));
    step_id_type aMatcherIndex = stepManager.addStepDefinition(a_matcher);
    EXPECT_EQ(aMatcherIndex, getUniqueMatchIdOrZeroFor(a_matcher));
    step_id_type anotherMatcherIndex = stepManager.addStepDefinition(another_matcher);
    EXPECT_EQ(anotherMatcherIndex, getUniqueMatchIdOrZeroFor(another_matcher));
    EXPECT_EQ(aMatcherIndex, getUniqueMatchIdOrZeroFor(a_matcher));
}

TEST_F(StepManagerTest, matchesStepsWithRegExMatchers) {
    stepManager.addStepDefinition("match the number (\\d+)");
    EXPECT_TRUE(matchesOnce("match the number 42"));
    EXPECT_FALSE(matchesOnce("match the number (\\d+)"));
    EXPECT_FALSE(matchesOnce("match the number one"));
}

TEST_F(StepManagerTest, extractsParamsFromRegExMatchers) {
    stepManager.addStepDefinition("match no params");
    EXPECT_TRUE(extractedParamsAre("match no params", no_params));
    stepManager.addStepDefinition("match the (\\w+) param");
    EXPECT_TRUE(extractedParamsAre("match the first param", map_list_of(10, "first")));
    stepManager.addStepDefinition("match a (.+)$");
    EXPECT_TRUE(extractedParamsAre("match a  string  with  spaces  ", map_list_of(8, " string  with  spaces  ")));
    stepManager.addStepDefinition("match params (\\w+), (\\w+) and (\\w+)");
    EXPECT_TRUE(extractedParamsAre("match params A, B and C", map_list_of(13, "A")(16, "B")(22, "C")));
}

TEST_F(StepManagerTest, handlesMultipleMatches) {
    stepManager.addStepDefinition(a_matcher);
    stepManager.addStepDefinition(another_matcher);
    stepManager.addStepDefinition(a_matcher);
    EXPECT_EQ(2, countMatches(a_matcher));
}

TEST_F(StepManagerTest, matchesStepsWithNonAsciiMatchers) {
    step_id_type aMatcherIndex = stepManager.addStepDefinition("خيار");
    EXPECT_EQ(aMatcherIndex, getUniqueMatchIdOrZeroFor("خيار"));
    EXPECT_FALSE(matchesAtLeastOnce("cetriolo"));
    EXPECT_FALSE(matchesAtLeastOnce("огурец"));
    EXPECT_FALSE(matchesAtLeastOnce("黄瓜"));
}
