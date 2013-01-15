#include <cucumber-cpp/internal/connectors/wire/WireProtocol.hpp>
#include <cucumber-cpp/internal/connectors/wire/WireProtocolCommands.hpp>

#include <gmock/gmock.h>
#include <boost/assign/list_of.hpp>

#include <typeinfo>

using namespace cuke::internal;
using namespace std;
using namespace testing;

using boost::assign::list_of;

class MockCukeEngine : public CukeEngine {
public:
    MOCK_CONST_METHOD1(stepMatches, std::vector<StepMatch>(const std::string & name));
    MOCK_METHOD1(endScenario, void(const tags_type & tags));
    MOCK_METHOD3(invokeStep, void(const std::string & id, const invoke_args_type & args, const invoke_table_type & tableArg));
    MOCK_METHOD1(beginScenario, void(const tags_type & tags));
    MOCK_CONST_METHOD3(snippetText, std::string(const std::string & keyword, const std::string & name, const std::string & multilineArgClass));
};

#define EXPECT_TYPE(classname, expression) \
    EXPECT_THAT(typeid(classname).name(), StrEq(typeid(expression).name()))

#define EXPECT_PTRTYPE(classname, expression) \
    EXPECT_TYPE(classname, *expression)



class WireMessageCodecTest : public Test {
protected:
    auto_ptr<WireCommand> commandAutoPtr;

    WireCommand *decode(const char *jsonStr) {
        commandAutoPtr = auto_ptr<WireCommand>(codec.decode(jsonStr));
        return commandAutoPtr.get();
    }

    const std::string encode(WireResponse *response) {
        auto_ptr<WireResponse> responseAutoPtr(response);
        return codec.encode(responseAutoPtr.get());
    }

protected:
    const JsonSpiritWireMessageCodec codec;
};


/*
 * Request decoding
 */

TEST_F(WireMessageCodecTest, decodesUnknownOrMalformedMessage) {
    MockCukeEngine engine;

    EXPECT_EQ(encode(decode("[\"unknown_command\"]")->run(&engine)), "[\"fail\"]");
    EXPECT_EQ(encode(decode("")->run(&engine)), "[\"fail\"]");
    EXPECT_EQ(encode(decode("rubbish")->run(&engine)), "[\"fail\"]");
    EXPECT_EQ(encode(decode("{\"malformed_command\"}")->run(&engine)), "[\"fail\"]");
    EXPECT_EQ(encode(decode("[42]")->run(&engine)), "[\"fail\"]");
}

TEST_F(WireMessageCodecTest, handlesStepMatchesMessage) {
    MockCukeEngine engine;
    EXPECT_CALL(engine, stepMatches("name to match"))
            .Times(1)
            .WillRepeatedly(Return(std::vector<StepMatch>(0)));

    decode("[\"step_matches\","
           "{\"name_to_match\":\"name to match\"}]")
            ->run(&engine);
}

TEST_F(WireMessageCodecTest, handlesBeginScenarioMessageWithoutArgument) {
    MockCukeEngine engine;
    EXPECT_CALL(engine, beginScenario(ElementsAre())).Times(1);

    decode("[\"begin_scenario\"]")->run(&engine);
}

TEST_F(WireMessageCodecTest, handlesBeginScenarioMessageWithTagsArgument) {
    MockCukeEngine engine;
    EXPECT_CALL(engine, beginScenario(ElementsAre("bar","baz","foo"))).Times(1);

    decode("[\"begin_scenario\","
           "{\"tags\":["
                "\"bar\","
                "\"baz\","
                "\"foo\""
           "]}]")->run(&engine);
}

TEST_F(WireMessageCodecTest, handlesBeginScenarioMessageWithNullArgument) {
    MockCukeEngine engine;
    EXPECT_CALL(engine, beginScenario(ElementsAre())).Times(1);

    decode("[\"begin_scenario\",null]")->run(&engine);
}

TEST_F(WireMessageCodecTest, handlesInvokeMessageWithNoArgs) {
    MockCukeEngine engine;
    EXPECT_CALL(engine, invokeStep("42", ElementsAre(), ElementsAre())).Times(1);

    decode("[\"invoke\",{\"id\":\"42\",\"args\":[]}]")->run(&engine);
}

TEST_F(WireMessageCodecTest, handlesInvokeMessageWithoutTableArgs) {
    MockCukeEngine engine;
    EXPECT_CALL(engine, invokeStep("42", ElementsAre("p1","p2","p3"), ElementsAre())).Times(1);

    decode("[\"invoke\",{"
           "\"id\":\"42\","
           "\"args\":["
                "\"p1\","
                "\"p2\","
                "\"p3\""
           "}]")->run(&engine);
}

TEST_F(WireMessageCodecTest, handlesInvokeMessageWithTableArgs) {
    MockCukeEngine engine;
    EXPECT_CALL(engine, invokeStep(
            "42",
            ElementsAre("p1"),
            ElementsAre(
                ElementsAre("col1","col2"),
                ElementsAre("r1c1","r1c2"),
                ElementsAre("r2c1","r2c2"),
                ElementsAre("r3c1","r3c2")
            )
        )).Times(1);

    decode("[\"invoke\",{"
           "\"id\":\"42\","
           "\"args\":["
                "\"p1\","
                "["
                    "[\"col1\",\"col2\"],"
                    "[\"r1c1\",\"r1c2\"],"
                    "[\"r2c1\",\"r2c2\"],"
                    "[\"r3c1\",\"r3c2\"]"
                "]"
           "}]")->run(&engine);
}

TEST_F(WireMessageCodecTest, handlesInvokeMessageWithNullArg) {
    MockCukeEngine engine;
    EXPECT_CALL(engine, invokeStep("42", ElementsAre(), ElementsAre())).Times(1);

    decode("[\"invoke\",{\"id\":\"42\",\"args\":[null]}]")->run(&engine);
}

TEST_F(WireMessageCodecTest, handlesEndScenarioMessageWithoutArgument) {
    MockCukeEngine engine;
    EXPECT_CALL(engine, endScenario(ElementsAre())).Times(1);

    decode("[\"end_scenario\"]")->run(&engine);
}

TEST_F(WireMessageCodecTest, handlesEndScenarioMessageWithNullArgument) {
    MockCukeEngine engine;
    EXPECT_CALL(engine, endScenario(ElementsAre())).Times(1);

    decode("[\"end_scenario\",null]")->run(&engine);
}

TEST_F(WireMessageCodecTest, handlesEndScenarioMessageWithTagsArgument) {
    MockCukeEngine engine;
    EXPECT_CALL(engine, endScenario(ElementsAre("cu","cum","ber"))).Times(1);

    decode("[\"end_scenario\","
           "{\"tags\":["
                "\"cu\","
                "\"cum\","
                "\"ber\""
           "]}]")->run(&engine);
}

TEST_F(WireMessageCodecTest, handlesSnippetTextMessage) {
    MockCukeEngine engine;
    EXPECT_CALL(engine, snippetText("Keyword", "step description","Some::Class")).Times(1);

    decode("[\"snippet_text\","
           "{\"step_keyword\":\"Keyword\","
             "\"multiline_arg_class\":\"Some::Class\","
             "\"step_name\":\"step description\"}]")
            ->run(&engine);
}

/*
 * Response encoding
 */

TEST_F(WireMessageCodecTest, handlesSuccessResponse) {
    SuccessResponse response;
    EXPECT_THAT(codec.encode(&response), StrEq("[\"success\"]"));
}

TEST_F(WireMessageCodecTest, handlesSimpleFailureResponse) {
    FailureResponse response;
    EXPECT_THAT(codec.encode(&response), StrEq("[\"fail\"]"));
}

TEST_F(WireMessageCodecTest, handlesDetailedFailureResponse) {
    FailureResponse response("My message","ExceptionClassName");
    EXPECT_THAT(codec.encode(&response), StrEq(
            "[\"fail\",{"
                "\"exception\":\"ExceptionClassName\","
                "\"message\":\"My message\""
            "}]"));
}

TEST_F(WireMessageCodecTest, handlesPendingResponse) {
    PendingResponse response("I'm lazy!");
    EXPECT_THAT(codec.encode(&response), StrEq("[\"pending\",\"I'm lazy!\"]"));
}

TEST_F(WireMessageCodecTest, handlesEmptyStepMatchesResponse) {
    StepMatchesResponse response(std::vector<StepMatch>(0));
    EXPECT_THAT(codec.encode(&response), StrEq("[\"success\",[]]"));
}

TEST_F(WireMessageCodecTest, handlesStepMatchesResponse) {
    std::vector<StepMatch> matches;
    StepMatch sm1;
    sm1.id = "1234";
    sm1.source = "MyClass.cpp:56";
    sm1.regexp = "Some (.*) regexp";
    matches.push_back(sm1);
    StepMatch sm2;
    sm2.id = "9876";
    StepMatchArg sm2arg1;
    sm2arg1.value = "odd";
    sm2arg1.position = 5;
    sm2.args.push_back(sm2arg1);
    matches.push_back(sm2);
    StepMatchesResponse response(matches);

    EXPECT_THAT(codec.encode(&response), StrEq(
            "[\"success\",[{"
                "\"args\":[],"
                "\"id\":\"1234\","
                "\"regexp\":\"Some (.*) regexp\","
                "\"source\":\"MyClass.cpp:56\""
            "},{"
                "\"args\":[{"
                    "\"pos\":5,"
                    "\"val\":\"odd\""
                "}],"
                "\"id\":\"9876\""
            "}]]"));
}

TEST_F(WireMessageCodecTest, handlesSnippetTextResponse) {
    SnippetTextResponse response("GIVEN(...)");
    EXPECT_THAT(codec.encode(&response), StrEq("[\"success\",\"GIVEN(...)\"]"));
}

/*
 * Command response
 */

TEST(WireCommandsTest, succesfulInvokeReturnsSuccess) {
    MockCukeEngine engine;
    InvokeCommand invokeCommand("x", new CukeEngine::invoke_args_type, new CukeEngine::invoke_table_type);
    EXPECT_CALL(engine, invokeStep(_, _, _))
            .Times(1);

    auto_ptr<const WireResponse> response(invokeCommand.run(&engine));
    EXPECT_PTRTYPE(SuccessResponse, response.get());
}

TEST(WireCommandsTest, throwingFailureInvokeReturnsFailure) {
    MockCukeEngine engine;
    InvokeCommand invokeCommand("x", new CukeEngine::invoke_args_type, new CukeEngine::invoke_table_type);
    EXPECT_CALL(engine, invokeStep(_, _, _))
            .Times(1)
            .WillOnce(Throw(InvokeFailureException("A", "B")));

    auto_ptr<const WireResponse> response(invokeCommand.run(&engine));
    EXPECT_PTRTYPE(FailureResponse, response.get());
    // TODO Test A and B
}

TEST(WireCommandsTest, throwingPendingStepReturnsPending) {
    MockCukeEngine engine;
    InvokeCommand invokeCommand("x", new CukeEngine::invoke_args_type, new CukeEngine::invoke_table_type);
    EXPECT_CALL(engine, invokeStep(_, _, _))
            .Times(1)
            .WillOnce(Throw(PendingStepException("S")));

    auto_ptr<const WireResponse> response(invokeCommand.run(&engine));
    EXPECT_PTRTYPE(PendingResponse, response.get());
    // TODO Test S
}

TEST(WireCommandsTest, throwingAnythingInvokeReturnsFailure) {
    MockCukeEngine engine;
    InvokeCommand invokeCommand("x", new CukeEngine::invoke_args_type, new CukeEngine::invoke_table_type);
    EXPECT_CALL(engine, invokeStep(_, _, _))
            .Times(1)
            .WillOnce(Throw(string("something")));

    auto_ptr<const WireResponse> response(invokeCommand.run(&engine));
    EXPECT_PTRTYPE(FailureResponse, response.get());
    // TODO Test empty
}






