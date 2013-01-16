#ifndef CUKE_CPPSPECDRIVER_HPP_
#define CUKE_CPPSPECDRIVER_HPP_

#include "../step/StepManager.hpp"

namespace cuke {
namespace internal {

class CppSpecStep : public BasicStep {
protected:
    const InvokeResult invokeStepBody();
};

#define STEP_INHERITANCE(step_name) ::cuke::internal::CppSpecStep, CppSpec::Specification<void, step_name >

}
}

#endif /* CUKE_CPPSPECDRIVER_HPP_ */
