#include <cucumber-cpp/internal/connectors/wire/WireProtocol.hpp>
#include <cucumber-cpp/internal/connectors/wire/WireProtocolCommands.hpp>

#include <json_spirit/json_spirit_reader_template.h>
#include <json_spirit/json_spirit_writer_template.h>

#include <boost/shared_ptr.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <string>
#include <sstream>

using namespace json_spirit;

namespace cuke {
namespace internal {


/*
 * Responses
 */


void SuccessResponse::accept(WireResponseVisitor *visitor) const {
    visitor->visit(this);
}

FailureResponse::FailureResponse(const std::string & message, const std::string & exceptionType) :
    message(message),
    exceptionType(exceptionType) {
}

const std::string FailureResponse::getMessage() const {
    return message;
}

const std::string FailureResponse::getExceptionType() const {
    return exceptionType;
}

void FailureResponse::accept(WireResponseVisitor *visitor) const {
    visitor->visit(this);
}

PendingResponse::PendingResponse(const std::string & message) :
    message(message) {
}

const std::string PendingResponse::getMessage() const {
    return message;
}

void PendingResponse::accept(WireResponseVisitor *visitor) const {
    visitor->visit(this);
}

StepMatchesResponse::StepMatchesResponse(const std::vector<StepMatch> & matchingSteps)
    : matchingSteps(matchingSteps) {
}

const std::vector<StepMatch> StepMatchesResponse::getMatchingSteps() const {
    return matchingSteps;
}

void StepMatchesResponse::accept(WireResponseVisitor *visitor) const {
    visitor->visit(this);
}

SnippetTextResponse::SnippetTextResponse(const std::string & stepSnippet) :
    stepSnippet(stepSnippet) {
}

const std::string SnippetTextResponse::getStepSnippet() const {
    return stepSnippet;
}

void SnippetTextResponse::accept(WireResponseVisitor *visitor) const {
    visitor->visit(this);
}


/*
 * Command decoders
 */


class CommandDecoder {
public:
    virtual WireCommand *decode(const mValue & jsonArgs) const = 0;
};


class ScenarioDecoder : public CommandDecoder {
protected:
    CukeEngine::tags_type *getTags(const mValue & jsonArgs) const {
        CukeEngine::tags_type *tags = new CukeEngine::tags_type;
        if (!jsonArgs.is_null()) {
            const mArray & jsonTags = jsonArgs.get_obj().at("tags").get_array();
            for (mArray::const_iterator i = jsonTags.begin(); i != jsonTags.end(); ++i) {
                tags->push_back(i->get_str());
            }
        }
        return tags;
    }
};


class BeginScenarioDecoder : public ScenarioDecoder {
public:
    WireCommand *decode(const mValue & jsonArgs) const {
        return new BeginScenarioCommand(getTags(jsonArgs));
    }
};


class EndScenarioDecoder : public ScenarioDecoder {
public:
    WireCommand *decode(const mValue & jsonArgs) const {
        return new EndScenarioCommand(getTags(jsonArgs));
    }
};


class StepMatchesDecoder : public CommandDecoder {
public:
    WireCommand *decode(const mValue & jsonArgs) const {
        mObject stepMatchesArgs(jsonArgs.get_obj());
        const std::string & nameToMatch(stepMatchesArgs["name_to_match"].get_str());
        return new StepMatchesCommand(nameToMatch);
    }
};


class InvokeDecoder : public CommandDecoder {
public:
    WireCommand *decode(const mValue & jsonArgs) const {
        mObject invokeParams(jsonArgs.get_obj());

        CukeEngine::invoke_args_type *args = new CukeEngine::invoke_args_type;
        CukeEngine::invoke_table_type *tableArg = new CukeEngine::invoke_table_type;
        const std::string & id(invokeParams["id"].get_str());
        fillInvokeArgs(invokeParams, *args, *tableArg);
        return new InvokeCommand(id, args, tableArg);
    }

private:
    void fillInvokeArgs(
            const mObject & invokeParams,
            CukeEngine::invoke_args_type & args,
            CukeEngine::invoke_table_type & tableArg) const {
        const mArray & jsonArgs(invokeParams.at("args").get_array());
        for (mArray::const_iterator i = jsonArgs.begin(); i != jsonArgs.end(); ++i) {
            if (i->type() == str_type) {
                args.push_back(i->get_str());
            } else if (i->type() == array_type) {
                fillTableArg(i->get_array(), tableArg);
            }
        }
    }

    void fillTableArg(const mArray & jsonTableArg, CukeEngine::invoke_table_type & tableArg) const {
        typedef mArray::size_type size_type;
        size_type rows = jsonTableArg.size();
        if (rows > 0) {
            size_type columns = jsonTableArg[0].get_array().size();
            tableArg.resize(boost::extents[rows][columns]);
            for (size_type i = 0; i < rows; ++i) {
                const mArray & jsonRow(jsonTableArg[i].get_array());
                if (jsonRow.size() == columns) {
                    for (size_type j = 0; j < columns; ++j) {
                        tableArg[i][j] = jsonRow[j].get_str();
                    }
                } else {
                    // TODO: Invalid row
                }
            }
        } else {
            // TODO: Invalid table (no column specified)
        }
    }
};


class SnippetTextDecoder : public CommandDecoder {
public:
    WireCommand *decode(const mValue & jsonArgs) const {
        mObject snippetTextArgs(jsonArgs.get_obj());
        const std::string & stepKeyword(snippetTextArgs["step_keyword"].get_str());
        const std::string & stepName(snippetTextArgs["step_name"].get_str());
        const std::string & multilineArgClass(snippetTextArgs["multiline_arg_class"].get_str());
        return new SnippetTextCommand(stepKeyword, stepName, multilineArgClass);
    }
};

static std::map<std::string, boost::shared_ptr<CommandDecoder> > commandDecodersMap = boost::assign::map_list_of
    ("begin_scenario", boost::shared_ptr<CommandDecoder> (new BeginScenarioDecoder))
    ("end_scenario", boost::shared_ptr<CommandDecoder> (new EndScenarioDecoder))
    ("step_matches", boost::shared_ptr<CommandDecoder> (new StepMatchesDecoder))
    ("invoke", boost::shared_ptr<CommandDecoder> (new InvokeDecoder))
    ("snippet_text", boost::shared_ptr<CommandDecoder> (new SnippetTextDecoder));


JsonSpiritWireMessageCodec::JsonSpiritWireMessageCodec() {};

WireCommand *JsonSpiritWireMessageCodec::decode(const std::string &request) const throw(WireMessageCodecException) {
    std::istringstream is(request);
    mValue json;
    try {
        read_stream(is, json);
        mArray & jsonRequest = json.get_array();
        mValue & jsonCommand = jsonRequest[0];

        CommandDecoder *commandDecoder = commandDecodersMap[jsonCommand.get_str()].get();
        if (commandDecoder != NULL) {
            mValue jsonArgs;
            if (jsonRequest.size() > 1) {
                jsonArgs = jsonRequest[1];
            }
            return commandDecoder->decode(jsonArgs);
        }
    } catch (...) {
        // LOG Error decoding wire protocol command
    }
    return new FailingCommand;
}

namespace {

    class WireResponseEncoder : public WireResponseVisitor {
    private:
        mArray jsonOutput;

        void success(const mValue *detail = 0) {
            output("success", detail);
        }

        void fail(const mValue *detail = 0) {
            output("fail", detail);
        }

        void output(const char *responseType, const mValue *detail = 0) {
            jsonOutput.push_back(responseType);
            if (detail == 0 || detail->is_null()) {
                return;
            }
            jsonOutput.push_back(*detail);
        }

    public:
        std::string encode(const WireResponse *response) {
            jsonOutput.clear();
            response->accept(this);
            const mValue v(jsonOutput);
            return write_string(v, false);
        }

        void visit(const SuccessResponse *response) {
            success();
        }

        void visit(const FailureResponse *response) {
            mObject detailObject;
            if (!response->getMessage().empty()) {
                detailObject["message"] = response->getMessage();
            }
            if (!response->getExceptionType().empty()) {
                detailObject["exception"] = response->getExceptionType();
            }
            if (detailObject.empty()) {
                fail();
            } else {
                const mValue detail(detailObject);
                fail(&detail);
            }
        }

        void visit(const PendingResponse *response) {
            mValue jsonReponse(response->getMessage());
            output("pending", &jsonReponse);
        }

        void visit(const StepMatchesResponse *response) {
            mArray jsonMatches;
            BOOST_FOREACH(StepMatch m, response->getMatchingSteps()) {
                mObject jsonM;
                jsonM["id"] = m.id;
                mArray jsonArgs;
                BOOST_FOREACH(StepMatchArg ma, m.args) {
                    mObject jsonMa;
                    jsonMa["val"] = ma.value;
                    jsonMa["pos"] = ma.position;
                    jsonArgs.push_back(jsonMa);
                }
                jsonM["args"] = jsonArgs;
                if (!m.source.empty()) {
                    jsonM["source"] = m.source;;
                }
                if (!m.regexp.empty()) {
                    jsonM["regexp"] = m.regexp;
                }
                jsonMatches.push_back(jsonM);
            }
            mValue jsonReponse(jsonMatches);
            output("success", &jsonReponse);
        }

        void visit(const SnippetTextResponse *response) {
            mValue jsonReponse(response->getStepSnippet());
            success(&jsonReponse);
        }
    };

}

const std::string JsonSpiritWireMessageCodec::encode(const WireResponse *response) const {
    try {
        WireResponseEncoder encoder;
        return encoder.encode(response);
    } catch (...) {
        throw WireMessageCodecException("Error decoding wire protocol response");
    }
}

WireProtocolHandler::WireProtocolHandler(const WireMessageCodec *codec, CukeEngine *engine) :
    codec(codec),
    engine(engine) {
}

std::string WireProtocolHandler::handle(const std::string &request) const {
    std::string response;
    // LOG request"
    try {
        const WireCommand *command = codec->decode(request);
        const WireResponse *wireResponse = command->run(engine);
        response = codec->encode(wireResponse);
    } catch (...) {
        response = "[\"fail\"]";
    }
    // LOG response
    return response;
}

}
}
