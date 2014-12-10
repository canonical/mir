#include "mir_toolkit/mir_client_library.h"
#include "mir_test_framework/connected_client_headless_server.h"
#include "mir_test_framework/stub_graphics_platform_operation.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstring>

namespace mtf = mir_test_framework;

namespace
{

void assign_reply(MirConnection*, MirPlatformMessage* reply, void* context)
{
    auto message = static_cast<MirPlatformMessage**>(context);
    *message = reply;
}

struct ClientPlatformOperation : mtf::ConnectedClientHeadlessServer
{
    MirPlatformMessage* platform_operation_add(int num1, int num2)
    {
        return platform_operation_add({num1,num2});
    }

    MirPlatformMessage* incorrect_platform_operation_add()
    {
        return platform_operation_add({7});
    }

    MirPlatformMessage* platform_operation_add(std::vector<int> const& nums)
    {
        auto const request = mir_platform_message_create(add_opcode);
        mir_platform_message_set_data(request, nums.data(), sizeof(int) * nums.size());
        MirPlatformMessage* reply;

        auto const platform_op_done = mir_connection_platform_operation(
            connection, add_opcode, request, assign_reply, &reply);
        mir_wait_for(platform_op_done);

        mir_platform_message_release(request);

        return reply;
    }

    unsigned int const add_opcode =
        static_cast<unsigned int>(mtf::StubGraphicsPlatformOperation::add);
};

MATCHER_P(MessageDataAsIntsEq, v, "")
{
    using namespace testing;
    auto msg_data = mir_platform_message_get_data(arg);
    if (msg_data.size % sizeof(int) != 0)
        throw std::runtime_error("Data is not an array of ints");

    std::vector<int> data(msg_data.size / sizeof(int));
    memcpy(data.data(), msg_data.data, msg_data.size);

    return v == data;
}

MATCHER(MessageDataIsEmpty, "")
{
    auto msg_data = mir_platform_message_get_data(arg);
    return msg_data.size == 0 && msg_data.data == nullptr;
}

MATCHER_P(MessageOpcodeEq, opcode, "")
{
    auto const msg_opcode = mir_platform_message_get_opcode(arg);
    return msg_opcode == opcode;
}

}

TEST_F(ClientPlatformOperation, exchanges_data_items_with_platform)
{
    using namespace testing;

    int const num1 = 7;
    int const num2 = 11;

    auto const reply = platform_operation_add(num1, num2);

    EXPECT_THAT(reply, MessageDataAsIntsEq(std::vector<int>{num1 + num2}));

    mir_platform_message_release(reply);
}

TEST_F(ClientPlatformOperation, does_not_set_connection_error_message_on_success)
{
    using namespace testing;

    auto const reply = platform_operation_add(7, 11);

    EXPECT_THAT(mir_connection_get_error_message(connection), StrEq(""));

    mir_platform_message_release(reply);
}

TEST_F(ClientPlatformOperation, reply_has_opcode_of_request)
{
    using namespace testing;

    auto const reply = platform_operation_add(7, 11);

    EXPECT_THAT(reply, MessageOpcodeEq(add_opcode));

    mir_platform_message_release(reply);
}

TEST_F(ClientPlatformOperation, returns_empty_reply_on_error)
{
    using namespace testing;

    auto const reply = incorrect_platform_operation_add();

    EXPECT_THAT(reply, MessageDataIsEmpty());

    mir_platform_message_release(reply);
}

TEST_F(ClientPlatformOperation, sets_connection_error_message_on_error)
{
    using namespace testing;

    auto const reply = incorrect_platform_operation_add();

    EXPECT_THAT(mir_connection_get_error_message(connection), StrNe(""));

    mir_platform_message_release(reply);
}
