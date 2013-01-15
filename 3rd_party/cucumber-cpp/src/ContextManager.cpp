#include "cucumber-cpp/internal/ContextManager.hpp"

namespace cuke {
namespace internal {

contexts_type ContextManager::contexts;

void ContextManager::purgeContexts() {
    contexts.clear();
}

}
}

