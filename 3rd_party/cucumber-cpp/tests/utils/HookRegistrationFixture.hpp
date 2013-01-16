#ifndef CUKE_HOOKREGISTRATIONFIXTURE_HPP_
#define CUKE_HOOKREGISTRATIONFIXTURE_HPP_

#include <cucumber-cpp/internal/hook/HookRegistrar.hpp>

#include "CukeCommandsFixture.hpp"

#include <sstream>

using namespace cuke::internal;

namespace {

std::stringstream beforeHookCallMarker;
std::stringstream aroundStepHookCallMarker;
std::stringstream beforeAroundStepHookCallMarker;
std::stringstream afterAroundStepHookCallMarker;
std::stringstream afterStepHookCallMarker;
std::stringstream afterHookCallMarker;
std::stringstream globalStepHookCallMarker;

void clearHookCallMarkers() {
    beforeHookCallMarker.str("");
    beforeAroundStepHookCallMarker.str("");
    afterAroundStepHookCallMarker.str("");
    afterStepHookCallMarker.str("");
    afterHookCallMarker.str("");
    globalStepHookCallMarker.str("");
}

std::string getHookCallMarkers() {
   return beforeHookCallMarker.str() +
           beforeAroundStepHookCallMarker.str() +
           afterStepHookCallMarker.str() +
           afterHookCallMarker.str();
}

class EmptyCallableStep : public CallableStep {
public:
    void call() {};
};

class HookRegistrarDouble : public HookRegistrar {
public:
    void execAroundStepHooks(Scenario *scenario) {
        EmptyCallableStep emptyStep;
        aroundhook_list_type &ash = aroundStepHooks();
        for (HookRegistrar::aroundhook_list_type::const_iterator h = ash.begin(); h != ash.end(); ++h) {
            (*h)->invokeHook(scenario, &emptyStep);
        }
    }
};

static const InvokeArgs NO_INVOKE_ARGS;

class HookRegistrationTest : public CukeCommandsFixture {
protected:
    HookRegistrarDouble hookRegistrar;
    shared_ptr<Scenario> emptyScenario;

    HookRegistrationTest() {
        emptyScenario = shared_ptr<Scenario>(new Scenario(new TagExpression::tag_list));
    }

    Scenario *getEmptyScenario() {
        return emptyScenario.get();
    }

    std::string beforeHookOrder() {
        clearHookCallMarkers();
        hookRegistrar.execBeforeHooks(getEmptyScenario());
        return getHookCallMarkers();
    }

    std::string aroundStepHookOrder() {
        clearHookCallMarkers();
        hookRegistrar.execAroundStepHooks(getEmptyScenario());
        return getHookCallMarkers();
    }

    std::string afterStepHookOrder() {
        clearHookCallMarkers();
        hookRegistrar.execAfterStepHooks(getEmptyScenario());
        return getHookCallMarkers();
    }

    std::string afterHookOrder() {
        clearHookCallMarkers();
        hookRegistrar.execAfterHooks(getEmptyScenario());
        return getHookCallMarkers();
    }

    void beginScenario(const TagExpression::tag_list *tags) {
        cukeCommands.beginScenario(tags);
    }

    void beginScenario(const TagExpression::tag_list & tags) {
        TagExpression::tag_list *pTags = new TagExpression::tag_list(tags.begin(), tags.end());
        beginScenario(pTags);
    }

    void invokeStep() {
        cukeCommands.invoke(stepInfoPtr->id, &NO_INVOKE_ARGS);
    }

    void endScenario() {
        cukeCommands.endScenario();
    }

    std::string sort(std::string str) {
        std::sort(str.begin(), str.end());
        return str;
    }

    void SetUp() {
        CukeCommandsFixture::SetUp();
        addStepToManager<EmptyStep>(STATIC_MATCHER);
    }

    void TearDown() {
        clearHookCallMarkers();
        CukeCommandsFixture::TearDown();
    }
};

}

#endif /* CUKE_HOOKREGISTRATIONFIXTURE_HPP_ */
