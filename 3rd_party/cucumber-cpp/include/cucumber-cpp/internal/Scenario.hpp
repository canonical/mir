#ifndef CUKE_SCENARIO_HPP_
#define CUKE_SCENARIO_HPP_

#include "hook/Tag.hpp"

namespace cuke {
namespace internal {

class Scenario {
public:
    Scenario(const TagExpression::tag_list *pTags);

    const TagExpression::tag_list & getTags();
private:
    shared_ptr<const TagExpression::tag_list> pTags;
};

}
}

#endif /* CUKE_SCENARIO_HPP_ */
