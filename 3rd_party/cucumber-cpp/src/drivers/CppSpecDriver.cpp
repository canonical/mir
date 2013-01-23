#include <cucumber-cpp/internal/drivers/CppSpecDriver.hpp>

#include <CppSpec/CppSpec.h>

namespace cuke {
namespace internal {

const InvokeResult CppSpecStep::invokeStepBody() {
    try {
        body();
        return InvokeResult::success();
    } catch (const ::CppSpec::SpecifyFailedException &e) {
        return InvokeResult::failure(e.message);
    }
}

}
}
