#include "cucumber-cpp/internal/CukeEngine.hpp"

namespace cuke {
namespace internal {

InvokeException::InvokeException(const std::string & message) :
    message(message) {
}

InvokeException::InvokeException(const InvokeException &rhs) :
    message(rhs.message) {
}

const std::string InvokeException::getMessage() const {
    return message;
}


InvokeFailureException::InvokeFailureException(const std::string & message, const std::string & exceptionType) :
        InvokeException(message),
        exceptionType(exceptionType) {
}

InvokeFailureException::InvokeFailureException(const InvokeFailureException &rhs) :
    InvokeException(rhs),
    exceptionType(rhs.exceptionType) {
}

const std::string InvokeFailureException::getExceptionType() const {
    return exceptionType;
}


PendingStepException::PendingStepException(const std::string & message) :
        InvokeException(message) {
}

PendingStepException::PendingStepException(const PendingStepException &rhs) :
    InvokeException(rhs) {
}

}
}
