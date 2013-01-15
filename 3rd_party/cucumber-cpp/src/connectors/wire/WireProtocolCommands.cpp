#include <cucumber-cpp/internal/connectors/wire/WireProtocolCommands.hpp>

namespace cuke {
namespace internal {

ScenarioCommand::ScenarioCommand(const CukeEngine::tags_type *tags) :
    tags(tags) {
}


BeginScenarioCommand::BeginScenarioCommand(const CukeEngine::tags_type *tags) :
    ScenarioCommand(tags) {
}

WireResponse *BeginScenarioCommand::run(CukeEngine *engine) const {
    engine->beginScenario(*tags);
    return new SuccessResponse;
}


EndScenarioCommand::EndScenarioCommand(const CukeEngine::tags_type *tags) :
    ScenarioCommand(tags) {
}

WireResponse *EndScenarioCommand::run(CukeEngine *engine) const {
    engine->endScenario(*tags);
    return new SuccessResponse;
}


StepMatchesCommand::StepMatchesCommand(const std::string & stepName) :
    stepName(stepName) {
}

WireResponse *StepMatchesCommand::run(CukeEngine *engine) const {
    std::vector<StepMatch> matchingSteps = engine->stepMatches(stepName);
    return new StepMatchesResponse(matchingSteps);
}


InvokeCommand::InvokeCommand(const std::string & stepId,
                             const CukeEngine::invoke_args_type *args,
                             const CukeEngine::invoke_table_type * tableArg) :
    stepId(stepId),
    args(args),
    tableArg(tableArg) {
}

WireResponse *InvokeCommand::run(CukeEngine *engine) const {
    try {
        engine->invokeStep(stepId, *args, *tableArg);
        return new SuccessResponse;
    } catch (InvokeFailureException e) {
        return new FailureResponse(e.getMessage(), e.getExceptionType());
    } catch (PendingStepException e) {
        return new PendingResponse(e.getMessage());
    } catch (...) {
        return new FailureResponse;
    }
}


SnippetTextCommand::SnippetTextCommand(const std::string & keyword, const std::string & name, const std::string & multilineArgClass) :
    keyword(keyword),
    name(name),
    multilineArgClass(multilineArgClass) {
}

WireResponse *SnippetTextCommand::run(CukeEngine *engine) const {
    return new SnippetTextResponse(engine->snippetText(keyword, name, multilineArgClass));
}


WireResponse *FailingCommand::run(CukeEngine *engine) const {
    return new FailureResponse;
}

}
}
