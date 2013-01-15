#ifndef CUKE_HOOKREGISTRAR_HPP_
#define CUKE_HOOKREGISTRAR_HPP_

#include "Tag.hpp"
#include "../Scenario.hpp"
#include "../step/StepManager.hpp"

#include <boost/smart_ptr.hpp>
using boost::shared_ptr;

#include <list>

namespace cuke {
namespace internal {

class CallableStep {
public:
    virtual void call() = 0;
};

class Hook {
public:
    void setTags(const std::string &csvTagNotation);
    virtual void invokeHook(Scenario *scenario);
    virtual void skipHook();
    virtual void body() = 0;
protected:
    bool tagsMatch(Scenario *scenario);
private:
    shared_ptr<TagExpression> tagExpression;
};

class BeforeHook : public Hook {
};

class AroundStepHook : public Hook {
public:
    virtual void invokeHook(Scenario *scenario, CallableStep *step);
    virtual void skipHook();
protected:
    CallableStep *step;
};

class AfterStepHook : public Hook {
};

class AfterHook : public Hook {
};


class HookRegistrar {
public:
    typedef std::list<Hook *> hook_list_type;
    typedef std::list<AroundStepHook *> aroundhook_list_type;

    virtual ~HookRegistrar();

    void addBeforeHook(BeforeHook *afterHook);
    void execBeforeHooks(Scenario *scenario);

    void addAroundStepHook(AroundStepHook *aroundStepHook);
    InvokeResult execStepChain(Scenario *scenario, StepInfo *stepInfo, const InvokeArgs *pArgs);

    void addAfterStepHook(AfterStepHook *afterStepHook);
    void execAfterStepHooks(Scenario *scenario);

    void addAfterHook(AfterHook *afterHook);
    void execAfterHooks(Scenario *scenario);

private:
    void execHooks(HookRegistrar::hook_list_type &hookList, Scenario *scenario);

protected:
    hook_list_type& beforeHooks();
    aroundhook_list_type& aroundStepHooks();
    hook_list_type& afterStepHooks();
    hook_list_type& afterHooks();
};


class StepCallChain {
public:
    StepCallChain(Scenario *scenario, StepInfo *stepInfo, const InvokeArgs *pStepArgs, HookRegistrar::aroundhook_list_type &aroundHooks);
    InvokeResult exec();
    void execNext();
private:
    void execStep();

    Scenario *scenario;
    StepInfo *stepInfo;
    const InvokeArgs *pStepArgs;

    HookRegistrar::aroundhook_list_type::iterator nextHook;
    HookRegistrar::aroundhook_list_type::iterator hookEnd;
    InvokeResult result;
};

class CallableStepChain : public CallableStep {
public:
    CallableStepChain(StepCallChain *scc);
    void call();
private:
    StepCallChain *scc;
};


template<class T>
static int registerBeforeHook(const std::string &csvTagNotation) {
   HookRegistrar reg;
   T *hook = new T;
   hook->setTags(csvTagNotation);
   reg.addBeforeHook(hook);
   return 0; // We are not interested in the ID at this time
}

template<class T>
static int registerAroundStepHook(const std::string &csvTagNotation) {
   HookRegistrar reg;
   T *hook = new T;
   hook->setTags(csvTagNotation);
   reg.addAroundStepHook(hook);
   return 0;
}

template<class T>
static int registerAfterStepHook(const std::string &csvTagNotation) {
   HookRegistrar reg;
   T *hook = new T;
   hook->setTags(csvTagNotation);
   reg.addAfterStepHook(hook);
   return 0;
}

template<class T>
static int registerAfterHook(const std::string &csvTagNotation) {
   HookRegistrar reg;
   T *hook = new T;
   hook->setTags(csvTagNotation);
   reg.addAfterHook(hook);
   return 0;
}

}
}

#endif /* CUKE_HOOKREGISTRAR_HPP_ */
