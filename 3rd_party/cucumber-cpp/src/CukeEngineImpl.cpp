#include "cucumber-cpp/internal/CukeEngineImpl.hpp"

#include <boost/foreach.hpp>

namespace cuke {
namespace internal {

namespace {

    std::string convertId(step_id_type id) {
        std::stringstream ss;
        ss << id;
        return ss.str();
    }

    step_id_type convertId(std::string stringid) {
        std::stringstream ss(stringid);
        step_id_type id;
        ss >> id;
        return id;
    }
}

std::vector<StepMatch> CukeEngineImpl::stepMatches(const std::string & name) const {
    std::vector<StepMatch> engineResult;
    MatchResult commandResult = cukeCommands.stepMatches(name);
    BOOST_FOREACH(const SingleStepMatch commandMatch, commandResult.getResultSet()) {
        StepMatch engineMatch;
        engineMatch.id = convertId(commandMatch.stepInfo->id);
        engineMatch.source = commandMatch.stepInfo->source;
        engineMatch.regexp = commandMatch.stepInfo->regex.str();
        BOOST_FOREACH(const RegexSubmatch commandMatchArg, commandMatch.submatches) {
            StepMatchArg engineMatchArg;
            engineMatchArg.value = commandMatchArg.value;
            engineMatchArg.position = commandMatchArg.position;
            engineMatch.args.push_back(engineMatchArg);
        }
        engineResult.push_back(engineMatch);
    }
    return engineResult;
}

void CukeEngineImpl::beginScenario(const tags_type & tags) {
    cukeCommands.beginScenario(&tags);
}

void CukeEngineImpl::invokeStep(const std::string & id, const invoke_args_type & args, const invoke_table_type & tableArg) {
    typedef invoke_table_type::index table_index;

    InvokeArgs commandArgs;
    try {
        BOOST_FOREACH(const std::string a, args) {
            commandArgs.addArg(a);
        }

        if (tableArg.shape()[0] > 1 && tableArg.shape()[1] > 0) {
            Table & commandTableArg = commandArgs.getVariableTableArg();
            for (table_index j = 0; j < tableArg.shape()[1]; ++j) {
                commandTableArg.addColumn(tableArg[0][j]);
            }

            for (table_index i = 1; i < tableArg.shape()[0]; ++i) {
                Table::row_type row;
                for (table_index j = 0; j < tableArg.shape()[1]; ++j) {
                    row.push_back(tableArg[i][j]);
                }
                commandTableArg.addRow(row);
            }
        }
    } catch (...) {
        throw InvokeException("Unable to decode arguments");
    }

    InvokeResult commandResult;
    try {
        commandResult = cukeCommands.invoke(convertId(id), &commandArgs);
    } catch (...) {
        throw InvokeException("Uncatched exception");
    }
    switch (commandResult.getType()) {
    case SUCCESS:
        return;
    case FAILURE:
        throw InvokeFailureException(commandResult.getDescription(), "");
    case PENDING:
        throw PendingStepException(commandResult.getDescription());
    }
}

void CukeEngineImpl::endScenario(const tags_type & tags) {
    cukeCommands.endScenario();
}

std::string CukeEngineImpl::snippetText(const std::string & keyword, const std::string & name, const std::string & multilineArgClass) const {
    return cukeCommands.snippetText(keyword, name);
}


}
}
