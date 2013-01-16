#include <gtest/gtest.h>

#include "../utils/HookRegistrationFixture.hpp"

#include <cucumber-cpp/internal/hook/HookMacros.hpp>


#define BEFORE_MARKER_1 "A"
#define BEFORE_MARKER_2 "B"
#define BEFORE_MARKER_3 "C"
BEFORE() { beforeHookCallMarker << BEFORE_MARKER_1; }
BEFORE() { beforeHookCallMarker << BEFORE_MARKER_2; }
BEFORE() { beforeHookCallMarker << BEFORE_MARKER_3; }

#define AROUND_STEP_MARKER_BEFORE_1 "D"
#define AROUND_STEP_MARKER_BEFORE_2 "E"
#define AROUND_STEP_MARKER_BEFORE_3 "F"
#define AROUND_STEP_MARKER_AFTER_3  "d"
#define AROUND_STEP_MARKER_AFTER_2  "e"
#define AROUND_STEP_MARKER_AFTER_1  "f"
AROUND_STEP() {
    beforeAroundStepHookCallMarker << AROUND_STEP_MARKER_BEFORE_1;
    globalStepHookCallMarker << AROUND_STEP_MARKER_BEFORE_1;
    step->call();
    globalStepHookCallMarker << AROUND_STEP_MARKER_AFTER_1;
    afterAroundStepHookCallMarker << AROUND_STEP_MARKER_AFTER_1;
}
AROUND_STEP() {
    beforeAroundStepHookCallMarker << AROUND_STEP_MARKER_BEFORE_2;
    globalStepHookCallMarker << AROUND_STEP_MARKER_BEFORE_2;
    step->call();
    globalStepHookCallMarker << AROUND_STEP_MARKER_AFTER_2;
    afterAroundStepHookCallMarker << AROUND_STEP_MARKER_AFTER_2;
}
AROUND_STEP() {
    beforeAroundStepHookCallMarker << AROUND_STEP_MARKER_BEFORE_3;
    globalStepHookCallMarker << AROUND_STEP_MARKER_BEFORE_3;
    step->call();
    globalStepHookCallMarker << AROUND_STEP_MARKER_AFTER_3;
    afterAroundStepHookCallMarker << AROUND_STEP_MARKER_AFTER_3;
}

#define AFTER_STEP_MARKER_1 "G"
#define AFTER_STEP_MARKER_2 "H"
#define AFTER_STEP_MARKER_3 "I"
AFTER_STEP() {
    afterStepHookCallMarker << AFTER_STEP_MARKER_1;
    globalStepHookCallMarker << AFTER_STEP_MARKER_1;
}
AFTER_STEP() {
    afterStepHookCallMarker << AFTER_STEP_MARKER_2;
    globalStepHookCallMarker << AFTER_STEP_MARKER_2;
}
AFTER_STEP() {
    afterStepHookCallMarker << AFTER_STEP_MARKER_3;
    globalStepHookCallMarker << AFTER_STEP_MARKER_3;
}

#define AFTER_MARKER_1 "J"
#define AFTER_MARKER_2 "K"
#define AFTER_MARKER_3 "L"
AFTER() { afterHookCallMarker << AFTER_MARKER_1; }
AFTER() { afterHookCallMarker << AFTER_MARKER_2; }
AFTER() { afterHookCallMarker << AFTER_MARKER_3; }


const std::string correctBeforeOrder(BEFORE_MARKER_1 BEFORE_MARKER_2 BEFORE_MARKER_3);
const std::string correctBeforeAroundStepOrder(AROUND_STEP_MARKER_BEFORE_1 AROUND_STEP_MARKER_BEFORE_2 AROUND_STEP_MARKER_BEFORE_3);
const std::string correctAfterAroundStepOrder(AROUND_STEP_MARKER_AFTER_3 AROUND_STEP_MARKER_AFTER_2 AROUND_STEP_MARKER_AFTER_1);
const std::string correctAfterStepOrder(AFTER_STEP_MARKER_1 AFTER_STEP_MARKER_2 AFTER_STEP_MARKER_3);
const std::string correctAfterOrder(AFTER_MARKER_1 AFTER_MARKER_2 AFTER_MARKER_3);


TEST_F(HookRegistrationTest, hooksAreRegisteredByTheMacros) {
    beginScenario(0);
    EXPECT_EQ(correctBeforeOrder, sort(beforeHookOrder()));
    EXPECT_EQ(correctBeforeAroundStepOrder, sort(aroundStepHookOrder()));
    EXPECT_EQ(correctAfterStepOrder, sort(afterStepHookOrder()));
    EXPECT_EQ(correctAfterOrder, sort(afterHookOrder()));
    endScenario();
}

TEST_F(HookRegistrationTest, beforeHooksAreInvokedInAnyOrder) {
    EXPECT_EQ("", beforeHookCallMarker.str());
    beginScenario(0);
    EXPECT_EQ(correctBeforeOrder, sort(beforeHookCallMarker.str()));
    invokeStep();
    endScenario();
    EXPECT_EQ(correctBeforeOrder, sort(beforeHookCallMarker.str()));
}

TEST_F(HookRegistrationTest, aroundStepHooksAreInvokedInAnyOrderButNested) {
    beginScenario(0);
    EXPECT_EQ("", beforeAroundStepHookCallMarker.str());
    invokeStep();
    EXPECT_EQ(correctBeforeAroundStepOrder, sort(beforeAroundStepHookCallMarker.str()));
    EXPECT_EQ(correctAfterAroundStepOrder, sort(afterAroundStepHookCallMarker.str()));
    endScenario();
    EXPECT_EQ(correctBeforeAroundStepOrder, sort(beforeAroundStepHookCallMarker.str()));
    EXPECT_EQ(correctAfterAroundStepOrder, sort(afterAroundStepHookCallMarker.str()));
}

TEST_F(HookRegistrationTest, afterStepHooksAreInvokedInAnyOrder) {
    beginScenario(0);
    EXPECT_EQ("", afterStepHookCallMarker.str());
    invokeStep();
    EXPECT_EQ(correctAfterStepOrder, sort(afterStepHookCallMarker.str()));
    endScenario();
    EXPECT_EQ(correctAfterStepOrder, sort(afterStepHookCallMarker.str()));
}

TEST_F(HookRegistrationTest, afterHooksAreInvokedInAnyOrder) {
    beginScenario(0);
    invokeStep();
    invokeStep();
    EXPECT_EQ("", sort(afterHookCallMarker.str()));
    endScenario();
    EXPECT_EQ(correctAfterOrder, sort(afterHookCallMarker.str()));
}

TEST_F(HookRegistrationTest, afterStepHooksAreInvokedAfterAroundStepHooks) {
    beginScenario(0);
    EXPECT_EQ("", globalStepHookCallMarker.str());
    invokeStep();
    EXPECT_EQ(beforeAroundStepHookCallMarker.str() +
              afterAroundStepHookCallMarker.str() +
              afterStepHookCallMarker.str(),
              globalStepHookCallMarker.str());
    endScenario();
}
