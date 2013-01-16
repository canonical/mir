#include <boost/test/unit_test.hpp>
#include <cucumber-cpp/defs.hpp>

#include "../../utils/DriverTestRunner.hpp"

THEN(SUCCEED_MATCHER) {
    USING_CONTEXT(cuke::internal::SomeContext, ctx);
    BOOST_CHECK(true);
}

THEN(FAIL_MATCHER) {
    USING_CONTEXT(cuke::internal::SomeContext, ctx);
    BOOST_CHECK(false);
}

THEN(PENDING_MATCHER_1) {
    pending();
}

THEN(PENDING_MATCHER_2) {
    pending(PENDING_DESCRIPTION);
}

using namespace cuke::internal;

class BoostStepDouble : public BoostStep {
public:
    const InvokeResult invokeStepBody() {
        return BoostStep::invokeStepBody();
    };

    void body() {};
};

class BoostDriverTest : public DriverTest {
public:
    virtual void runAllTests() {
        stepInvocationInitsBoostTest();
        DriverTest::runAllTests();
    }

private:
    void stepInvocationInitsBoostTest() {
        std::cout << "= Init =" << std::endl;
        using namespace boost::unit_test;
        BoostStepDouble step;
        expectFalse("Framework is not initialized before the first test", framework::is_initialized());
	step.invokeStepBody();
        expectTrue("Framework is initialized after the first test", framework::is_initialized());
    }
};

int main(int argc, char **argv) {
    BoostDriverTest test;
    return test.run();
}
