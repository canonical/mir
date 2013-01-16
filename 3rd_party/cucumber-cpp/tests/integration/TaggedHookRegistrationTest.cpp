#include <gtest/gtest.h>

#include "../utils/HookRegistrationFixture.hpp"
#include <cucumber-cpp/internal/hook/HookMacros.hpp>

#include <boost/assign.hpp>
using boost::assign::list_of;

BEFORE("@a") { beforeHookCallMarker << "A"; }
BEFORE("@a","@b") { beforeHookCallMarker << "B"; }
BEFORE("@a,@b") { beforeHookCallMarker << "C"; }

AROUND_STEP("@a") { afterAroundStepHookCallMarker << "D"; step->call(); }
AROUND_STEP("@a","@b") { afterAroundStepHookCallMarker << "E"; step->call(); }
AROUND_STEP("@a,@b") { afterAroundStepHookCallMarker << "F"; step->call(); }

AFTER_STEP("@a") { afterStepHookCallMarker << "G"; }
AFTER_STEP("@a","@b") { afterStepHookCallMarker << "H"; }
AFTER_STEP("@a,@b") { afterStepHookCallMarker << "I"; }

AFTER("@a") { afterHookCallMarker << "J"; }
AFTER("@a","@b") { afterHookCallMarker << "K"; }
AFTER("@a,@b") { afterHookCallMarker << "L"; }


TEST_F(HookRegistrationTest, noTaggedHooksAreInvokedIfNoScenarioTag) {
    beginScenario(0);
    invokeStep();
    endScenario();
    EXPECT_EQ("", sort(beforeHookCallMarker.str()));
    EXPECT_EQ("", sort(afterAroundStepHookCallMarker.str()));
    EXPECT_EQ("", sort(afterStepHookCallMarker.str()));
    EXPECT_EQ("", sort(afterHookCallMarker.str()));
}

TEST_F(HookRegistrationTest, orTagsAreEnforced) {
    const TagExpression::tag_list tags = list_of("b");
    beginScenario(tags);
    invokeStep();
    endScenario();
    EXPECT_EQ("C", sort(beforeHookCallMarker.str()));
    EXPECT_EQ("F", sort(afterAroundStepHookCallMarker.str()));
    EXPECT_EQ("I", sort(afterStepHookCallMarker.str()));
    EXPECT_EQ("L", sort(afterHookCallMarker.str()));
}

TEST_F(HookRegistrationTest, andTagsAreEnforced) {
    const TagExpression::tag_list tags = list_of("a")("b");
    beginScenario(tags);
    invokeStep();
    endScenario();
    EXPECT_EQ("ABC", sort(beforeHookCallMarker.str()));
    EXPECT_EQ("DEF", sort(afterAroundStepHookCallMarker.str()));
    EXPECT_EQ("GHI", sort(afterStepHookCallMarker.str()));
    EXPECT_EQ("JKL", sort(afterHookCallMarker.str()));
}
