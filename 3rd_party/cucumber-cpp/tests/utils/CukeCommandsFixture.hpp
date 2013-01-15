#ifndef CUKE_CUKECOMMANDSFIXTURE_HPP_ 
#define CUKE_CUKECOMMANDSFIXTURE_HPP_

#include <cucumber-cpp/internal/CukeCommands.hpp>
#include <cucumber-cpp/internal/drivers/FakeDriver.hpp>
#include "StepManagerTestDouble.hpp"

#include <boost/shared_ptr.hpp>

using namespace cuke::internal;
using boost::shared_ptr;

class EmptyStep : public FakeStep {
    void body() {}
};

class CukeCommandsFixture : public ::testing::Test {
    StepManagerTestDouble stepManager;
public:
    const static std::string STATIC_MATCHER;

protected:
    CukeCommands cukeCommands;
    shared_ptr<StepInfo> stepInfoPtr;

    template<class T>
    void runStepBodyTest() {
        addStepToManager<T>(STATIC_MATCHER);
        const InvokeArgs *pArgs = T::buildInvokeArgs();
        shared_ptr<const InvokeArgs> spArgs(pArgs);
        cukeCommands.invoke(stepInfoPtr->id, pArgs);
    }

    template<class T>
    void addStepToManager(const std::string &matcher) {
        stepInfoPtr = shared_ptr<StepInfo>(new StepInvoker<T>(matcher, ""));
        stepManager.addStep(stepInfoPtr.get());
    }

    virtual void TearDown() {
        stepManager.clearSteps();
    }
};

const std::string CukeCommandsFixture::STATIC_MATCHER("MATCHER");

#endif /* CUKE_CUKECOMMANDSFIXTURE_HPP_ */

