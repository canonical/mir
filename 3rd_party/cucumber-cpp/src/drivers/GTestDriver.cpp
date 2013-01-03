#include <cucumber-cpp/internal/drivers/GTestDriver.hpp>

#include <gtest/gtest.h>

namespace cuke {
namespace internal {

bool GTestStep::initialized(false);

const InvokeResult GTestStep::invokeStepBody() {
    if (!initialized) {
        initGTest();
        initFlags();
    }
    try {
        body();
        return InvokeResult::success();
    } catch (const ::std::runtime_error &e) {
        // ::testing::GoogleTestFailureException inherits from ::std::runtime_error
        return InvokeResult::failure(e.what());
    }
}

void GTestStep::initGTest() {
    int fake_argc = 1;
    char *fake_argv[1];
    fake_argv[0] = (char *) "cucumber-cpp";
    ::testing::InitGoogleTest(&fake_argc, fake_argv);
    initialized = true;
}

void GTestStep::initFlags() {
    ::testing::GTEST_FLAG(throw_on_failure) = true;  // let cucumber-cpp drive
    ::testing::GTEST_FLAG(break_on_failure) = false; // turn off debugger breakpoints
    ::testing::GTEST_FLAG(catch_exceptions) = true;
}

}
}
