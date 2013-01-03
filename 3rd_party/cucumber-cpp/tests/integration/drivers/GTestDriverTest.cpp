#include <gtest/gtest.h>
#include <cucumber-cpp/defs.hpp>

#include "../../utils/DriverTestRunner.hpp"

THEN(SUCCEED_MATCHER) {
    USING_CONTEXT(cuke::internal::SomeContext, ctx);
    ASSERT_TRUE(true);
}

THEN(FAIL_MATCHER) {
    USING_CONTEXT(cuke::internal::SomeContext, ctx);
    ASSERT_TRUE(false);
}

THEN(PENDING_MATCHER_1) {
    pending();
}

THEN(PENDING_MATCHER_2) {
    pending(PENDING_DESCRIPTION);
}

using namespace cuke::internal;

class GTestStepDouble : public GTestStep {
public:
    bool isInitialized() {
        return GTestStep::initialized;
    }

    const InvokeResult invokeStepBody() {
        return GTestStep::invokeStepBody();
    };

    void body() {};
};

class GTestDriverTest : public DriverTest {
public:
    virtual void runAllTests() {
        stepInvocationInitsGTest();
        DriverTest::runAllTests();
    }

private:
    void stepInvocationInitsGTest() {
        std::cout << "= Init =" << std::endl;
        GTestStepDouble framework;
        expectFalse("Framework is not initialized before the first test", framework.isInitialized());
        framework.invokeStepBody();
        expectTrue("Framework is initialized after the first test", framework.isInitialized());
    }
};

int main() {
    GTestDriverTest test;
    return test.run();
}
