#ifndef CUKE_BOOSTDRIVER_HPP_
#define CUKE_BOOSTDRIVER_HPP_

#include "../step/StepManager.hpp"

namespace cuke {
namespace internal {

class CukeBoostLogInterceptor;

class BoostStep : public BasicStep {
protected:
    const InvokeResult invokeStepBody();

private:
    void initBoostTest();
    void runWithMasterSuite();
};

#define STEP_INHERITANCE(step_name) ::cuke::internal::BoostStep

}
}

#endif /* CUKE_BOOSTDRIVER_HPP_ */
