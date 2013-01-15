#include "cucumber-cpp/internal/CukeCommands.hpp"

#include <sstream>
#include <boost/algorithm/string.hpp>

namespace cuke {
namespace internal {

shared_ptr<Scenario> CukeCommands::currentScenario;

void CukeCommands::beginScenario(const TagExpression::tag_list *tags) {
    currentScenario = shared_ptr<Scenario>(new Scenario(tags));
    hookRegistrar.execBeforeHooks(currentScenario.get());
}

void CukeCommands::endScenario() {
    hookRegistrar.execAfterHooks(currentScenario.get());
    contextManager.purgeContexts();
    currentScenario.reset();
}

const std::string CukeCommands::snippetText(const std::string stepKeyword, const std::string stepName) const {
    std::stringstream snippetText; // TODO Escape stepName
    snippetText << boost::to_upper_copy(stepKeyword) << "(\"^" << stepName << "$\") {" << std::endl;
    snippetText << "    pending();" << std::endl;
    snippetText << "}" << std::endl;
    return snippetText.str();
}

MatchResult CukeCommands::stepMatches(const std::string description) const {
    return stepManager.stepMatches(description);
}

InvokeResult CukeCommands::invoke(step_id_type id, const InvokeArgs *pArgs) {
    StepInfo *stepInfo = stepManager.getStep(id);
    InvokeResult result = hookRegistrar.execStepChain(currentScenario.get(), stepInfo, pArgs);
    hookRegistrar.execAfterStepHooks(currentScenario.get());
    return result;
}

}
}
