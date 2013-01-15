#ifndef CUKE_DRIVERTESTRUNNER_HPP_
#define CUKE_DRIVERTESTRUNNER_HPP_

#include "StepManagerTestDouble.hpp"
#include <cucumber-cpp/internal/CukeCommands.hpp>

#include <iostream>
#include <string>

namespace cuke {
namespace internal {

class ContextListener {
private:
    static int createdContexts;
    static int destroyedContexts;
public:
    int getCreatedContexts() {
        return createdContexts;
    }
    int getDestroyedContexts() {
        return destroyedContexts;
    }
    void notifyCreation() {
        ++createdContexts;
    }
    void notifyDestruction() {
        ++destroyedContexts;
    }
    void reset() {
        createdContexts = 0;
        destroyedContexts = 0;
    }
};

int ContextListener::createdContexts = 0;
int ContextListener::destroyedContexts = 0;


class SomeContext {
private:
    ContextListener listener;
public:
    SomeContext() {
        listener.notifyCreation();
    }
    ~SomeContext() {
        listener.notifyDestruction();
    }
};

static const InvokeArgs NO_INVOKE_ARGS;

#define SUCCEED_MATCHER   "Succeeding step"
#define FAIL_MATCHER      "Failing step"
#define PENDING_MATCHER_1 "Pending step without description"
#define PENDING_MATCHER_2 "Pending step with description"

#define PENDING_DESCRIPTION    "Describe me!"

class DriverTest {
public:
    int run() {
        runAllTests();
        return failedTests;
    }

    DriverTest() {
        failedTests = 0;
    }
protected:
    void expectTrue(const char *description, bool condition) {
        updateState(description, condition);
    }

    void expectFalse(const char *description, bool condition) {
        updateState(description, !condition);
    }

    template<typename T>
    void expectEqual(const char *description, T val1, T val2) {
        updateState(description, val1 == val2);
    }

    void expectStrEqual(const char *description, const char *val1, const char *val2) {
        updateState(description, strcmp(val1, val2) == 0);
    }

    virtual void runAllTests() {
        invokeRunsTests();
        contextConstructorAndDesctructorGetCalled();
        failureDescriptionIsResetOnEachRun();
    }

    CukeCommands cukeCommands;
private:
    StepManagerTestDouble stepManager;
    ContextListener listener;

    int failedTests;

    void updateState(const char *description, bool testSuccessState) {
        std::cout << (testSuccessState ? "SUCCESS" : "FAILURE")
                  << " (" << description << ")" << std::endl;
        failedTests += testSuccessState ? 0 : 1;
    }

    step_id_type getStepIdFromMatcher(const std::string &stepMatcher) {
        return stepManager.getStepId(stepMatcher);
    }

    void invokeRunsTests() {
        std::cout << "= Step invocation =" << std::endl;

        InvokeResult result;

        cukeCommands.beginScenario(0);

        result = cukeCommands.invoke(getStepIdFromMatcher(SUCCEED_MATCHER), &NO_INVOKE_ARGS);
        expectTrue("Succeeding step", result.isSuccess());

        result = cukeCommands.invoke(getStepIdFromMatcher(FAIL_MATCHER), &NO_INVOKE_ARGS);
        expectFalse("Failing step", result.isSuccess() || result.isPending());
        expectFalse("Failing step has a message", result.getDescription().empty());

        result = cukeCommands.invoke(getStepIdFromMatcher(PENDING_MATCHER_1), &NO_INVOKE_ARGS);
        expectTrue("Pending step with no description - result", result.isPending());
        expectStrEqual("Pending step with no description - description", "", result.getDescription().c_str());

        result = cukeCommands.invoke(getStepIdFromMatcher(PENDING_MATCHER_2), &NO_INVOKE_ARGS);
        expectTrue("Pending step with description - result", result.isPending());
        expectStrEqual("Pending step with description - description", PENDING_DESCRIPTION, result.getDescription().c_str());

        result = cukeCommands.invoke(42, &NO_INVOKE_ARGS);
        expectFalse("Inexistent step", result.isSuccess());

        cukeCommands.endScenario();
    }

    void contextConstructorAndDesctructorGetCalled() {
        std::cout << "= Constructors and destructors =" << std::endl;
        contextConstructorAndDesctructorGetCalledOn(SUCCEED_MATCHER);
        contextConstructorAndDesctructorGetCalledOn(FAIL_MATCHER);
    }

    void contextConstructorAndDesctructorGetCalledOn(const std::string stepMatcher) {
        std::cout << "== " << stepMatcher << " ==" << std::endl;
        listener.reset();
        cukeCommands.beginScenario(0);
        cukeCommands.invoke(getStepIdFromMatcher(stepMatcher), &NO_INVOKE_ARGS);
        expectEqual("Contexts created after invoke", 1, listener.getCreatedContexts());
        expectEqual("Contexts destroyed after invoke", 0, listener.getDestroyedContexts());
        cukeCommands.endScenario();
        expectEqual("Contexts created after end scenario", 1, listener.getCreatedContexts());
        expectEqual("Contexts destroyed after end scenario", 1, listener.getDestroyedContexts());
    }

    void failureDescriptionIsResetOnEachRun() {
        std::cout << "= Step failure description is reset =" << std::endl;
        InvokeResult result;
        cukeCommands.beginScenario(0);

        result = cukeCommands.invoke(getStepIdFromMatcher(FAIL_MATCHER), &NO_INVOKE_ARGS);
        std::string failureMessage = result.getDescription();

        expectFalse("Failing step description is set", failureMessage.empty());

        result = cukeCommands.invoke(getStepIdFromMatcher(FAIL_MATCHER), &NO_INVOKE_ARGS);

        expectEqual("Failing step description is the same", failureMessage, result.getDescription());
    }
};

}
}

#endif /* CUKE_DRIVERTESTRUNNER_HPP_ */
