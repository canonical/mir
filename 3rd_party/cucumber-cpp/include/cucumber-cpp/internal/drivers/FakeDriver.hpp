#ifndef CUKE_FAKEDRIVER_HPP_
#define CUKE_FAKEDRIVER_HPP_

#include "../step/StepManager.hpp"

namespace cuke {
namespace internal {

class FakeStep : public BasicStep {
protected:
    const InvokeResult invokeStepBody();
};

#define STEP_INHERITANCE(step_name) ::cuke::internal::FakeStep


const InvokeResult FakeStep::invokeStepBody() {
    // No try/catch block to throw the original exceptions to the testing framework
    body();
    return InvokeResult::success();
}

}
}

#endif /* CUKE_FAKEDRIVER_HPP_ */
