#include <cucumber-cpp/internal/Scenario.hpp>

namespace cuke {
namespace internal {

Scenario::Scenario(const TagExpression::tag_list *pTags) :
    pTags(pTags) {
    if (!pTags) {
        this->pTags = shared_ptr<const TagExpression::tag_list>(new TagExpression::tag_list);
    }
};

const TagExpression::tag_list & Scenario::getTags() {
    return *(pTags.get());
}

}
}
