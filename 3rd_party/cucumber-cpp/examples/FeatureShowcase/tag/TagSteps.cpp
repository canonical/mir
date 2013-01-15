#include <gtest/gtest.h>
#include <cucumber-cpp/defs.hpp>

#include <iostream>
using namespace std;


BEFORE() {
    cout << "-------------------- (Before any scenario)" << endl;
}

BEFORE("@foo,@bar","@baz") {
    cout << "Before scenario (\"@foo,@baz\",\"@bar\")" << endl;
}

AROUND_STEP("@baz") {
    cout << "Around step (\"@baz\") ...before" << endl;
    step->call();
    cout << "Around step (\"@baz\") ..after" << endl;
}

AFTER_STEP("@bar") {
    cout << "After step (\"@bar\")" << endl;
}

AFTER("@foo") {
    cout << "After scenario (\"@foo\")" << endl;
}

AFTER("@gherkin") {
    cout << "After scenario (\"@gherkin\")" << endl;
}


/*
 * CUKE_STEP_ is used just because the feature does not convey any
 * business value. It should NEVER be used in real step definitions!
 */

CUKE_STEP_("^I'm running a step from a scenario not tagged$") {
    cout << "Running step from scenario without tags" << endl;
    EXPECT_TRUE(true); // To fix a problem at link time
}

CUKE_STEP_("^I'm running a step from a scenario tagged with (.*)$") {
    REGEX_PARAM(string, tags);
    cout << "Running step from scenario with tags: " << tags << endl;
    EXPECT_TRUE(true); // To fix a problem at link time
}
