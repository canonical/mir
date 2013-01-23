#ifndef CUKE_STEPMANAGERTESTDOUBLE_HPP_
#define CUKE_STEPMANAGERTESTDOUBLE_HPP_

#include <cucumber-cpp/internal/step/StepManager.hpp>

namespace cuke {
namespace internal {


class StepInfoNoOp : public StepInfo {
public:
    StepInfoNoOp(const std::string &stepMatcher, const std::string source) : StepInfo(stepMatcher, source) {}
    InvokeResult invokeStep(const InvokeArgs *pArgs) {
        return InvokeResult::success();
    }
};

class StepInfoPending : public StepInfo {
private:
    const char *description;
public:
    StepInfoPending(const std::string &stepMatcher, const char *description) :
        StepInfo(stepMatcher, ""),
        description(description) {
    }

    InvokeResult invokeStep(const InvokeArgs *pArgs) {
        return InvokeResult::pending(description);
    }
};

/*
 * FIXME This should be a mock testing it receives a table argument
 */
class StepInfoWithTableArg : public StepInfo {
    const unsigned short expectedSize;
public:
    StepInfoWithTableArg(const std::string &stepMatcher, const unsigned short expectedSize) :
        StepInfo(stepMatcher, ""),
        expectedSize(expectedSize) {
    }

    InvokeResult invokeStep(const InvokeArgs *pArgs) {
        if (pArgs->getTableArg().hashes().size() == expectedSize) {
            return InvokeResult::success();
        } else {
            return InvokeResult::failure();
        }
    }
};


class StepManagerTestDouble : public StepManager {
public:
    void clearSteps() {
        steps().clear();
    }

    steps_type::size_type count() {
        return steps().size();
    }

    step_id_type addStepDefinition(const std::string &stepMatcher) {
        StepInfo *stepInfo = new StepInfoNoOp(stepMatcher, "");
        addStep(stepInfo);
        return stepInfo->id;
    }

    void addStepDefinitionWithId(step_id_type desiredId, const std::string &stepMatcher) {
        addStepDefinitionWithId(desiredId, stepMatcher, "");
    }

    void addStepDefinitionWithId(step_id_type desiredId, const std::string &stepMatcher,
            const std::string source) {
        StepInfo *stepInfo = new StepInfoNoOp(stepMatcher, source);
        stepInfo->id = desiredId;
        addStep(stepInfo);
    }

    void addPendingStepDefinitionWithId(step_id_type desiredId, const std::string &stepMatcher) {
        addPendingStepDefinitionWithId(desiredId, stepMatcher, 0);
    }

    void addPendingStepDefinitionWithId(step_id_type desiredId,
            const std::string &stepMatcher, const char *description) {
        StepInfo *stepInfo = new StepInfoPending(stepMatcher, description);
        stepInfo->id = desiredId;
        addStep(stepInfo);
    }

    void addTableStepDefinitionWithId(step_id_type desiredId, const std::string &stepMatcher, const unsigned short expectedSize) {
        StepInfo *stepInfo = new StepInfoWithTableArg(stepMatcher, expectedSize);
        stepInfo->id = desiredId;
        addStep(stepInfo);
    }

    const step_id_type getStepId(const std::string &stepMatcher) {
        step_id_type id = 0;
        for (steps_type::const_iterator i = steps().begin(); i != steps().end(); ++i) {
            StepInfo *stepInfo = i->second;
            if (stepInfo->regex.str() == stepMatcher) {
                id = stepInfo->id;
                break;
            }
        }
        return id;
    }
};

}
}

#endif /* CUKE_STEPMANAGERTESTDOUBLE_HPP_ */
