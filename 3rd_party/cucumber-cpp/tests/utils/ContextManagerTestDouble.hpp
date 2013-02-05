#ifndef CUKE_CONTEXTMANAGERTESTDOUBLE_HPP_
#define CUKE_CONTEXTMANAGERTESTDOUBLE_HPP_

#include <cucumber-cpp/internal/ContextManager.hpp>

namespace cuke {
namespace internal {

class ContextManagerTestDouble : public ContextManager {
public:
    contexts_type::size_type countContexts() {
        return ContextManager::contexts.size();
    }
};

}
}

#endif /* CUKE_CONTEXTMANAGERTESTDOUBLE_HPP_ */
