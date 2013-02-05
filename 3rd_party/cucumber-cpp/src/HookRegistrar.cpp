#include <cucumber-cpp/internal/hook/HookRegistrar.hpp>

#include <cucumber-cpp/internal/CukeCommands.hpp>

namespace cuke {
namespace internal {

void Hook::invokeHook(Scenario *scenario) {
    if (tagsMatch(scenario)) {
        body();
    } else {
        skipHook();
    }
}

void Hook::skipHook() {
}

void Hook::setTags(const std::string &csvTagNotation) {
    tagExpression = shared_ptr<TagExpression>(new AndTagExpression(csvTagNotation));
}

bool Hook::tagsMatch(Scenario *scenario) {
    return !scenario || tagExpression->matches(scenario->getTags());
}

void AroundStepHook::invokeHook(Scenario *scenario, CallableStep *step) {
    this->step = step;
    Hook::invokeHook(scenario);
}

void AroundStepHook::skipHook() {
    step->call();
}


HookRegistrar::~HookRegistrar() {
}

void HookRegistrar::addBeforeHook(BeforeHook *beforeHook) {
    beforeHooks().push_back(beforeHook);
}

HookRegistrar::hook_list_type& HookRegistrar::beforeHooks() {
    static hook_list_type *beforeHooks = new hook_list_type();
    return *beforeHooks;
}

void HookRegistrar::execBeforeHooks(Scenario *scenario) {
    execHooks(beforeHooks(), scenario);
}


void HookRegistrar::addAroundStepHook(AroundStepHook *aroundStepHook) {
    aroundStepHooks().push_front(aroundStepHook);
}

HookRegistrar::aroundhook_list_type& HookRegistrar::aroundStepHooks() {
    static aroundhook_list_type *aroundStepHooks = new aroundhook_list_type();
    return *aroundStepHooks;
}

InvokeResult HookRegistrar::execStepChain(Scenario *scenario, StepInfo *stepInfo, const InvokeArgs *pArgs) {
    StepCallChain scc(scenario, stepInfo, pArgs, aroundStepHooks());
    return scc.exec();
}


void HookRegistrar::addAfterStepHook(AfterStepHook *afterStepHook) {
    afterStepHooks().push_front(afterStepHook);
}

HookRegistrar::hook_list_type& HookRegistrar::afterStepHooks() {
    static hook_list_type *afterStepHooks = new hook_list_type();
    return *afterStepHooks;
}

void HookRegistrar::execAfterStepHooks(Scenario *scenario) {
    execHooks(afterStepHooks(), scenario);
}


void HookRegistrar::addAfterHook(AfterHook *afterHook) {
    afterHooks().push_front(afterHook);
}

HookRegistrar::hook_list_type& HookRegistrar::afterHooks() {
    static hook_list_type *afterHooks = new hook_list_type();
    return *afterHooks;
}

void HookRegistrar::execAfterHooks(Scenario *scenario) {
    execHooks(afterHooks(), scenario);
}


void HookRegistrar::execHooks(HookRegistrar::hook_list_type &hookList, Scenario *scenario) {
    for (HookRegistrar::hook_list_type::iterator hook = hookList.begin(); hook != hookList.end(); ++hook) {
        (*hook)->invokeHook(scenario);
    }
}


StepCallChain::StepCallChain(
    Scenario *scenario,
    StepInfo *stepInfo,
    const InvokeArgs *pStepArgs,
    HookRegistrar::aroundhook_list_type &aroundHooks
) :
    scenario(scenario),
    stepInfo(stepInfo),
    pStepArgs(pStepArgs)
{
    nextHook = aroundHooks.begin();
    hookEnd = aroundHooks.end();
}

InvokeResult StepCallChain::exec() {
    execNext();
    return result;
}

void StepCallChain::execNext() {
    if (nextHook == hookEnd) {
        execStep();
    } else {
        HookRegistrar::aroundhook_list_type::iterator currentHook = nextHook++;
        CallableStepChain callableStepChain(this);
        (*currentHook)->invokeHook(scenario, &callableStepChain);
    }
}

void StepCallChain::execStep() {
    if (stepInfo) {
        result = stepInfo->invokeStep(pStepArgs);
    }
}


CallableStepChain::CallableStepChain(StepCallChain *scc) : scc(scc) {};

void CallableStepChain::call() {
    scc->execNext();
}


}
}
