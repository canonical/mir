#include "mir_toolkit/mir_client_library.h"
#include "mir_test_framework/connected_client_headless_server.h"
#include "mir_test_framework/stub_graphics_platform_operation.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstring>

namespace mtf = mir_test_framework;

namespace
{

void assign_package(MirConnection*, MirPlatformPackage const* reply, void* context)
{
    auto package = static_cast<MirPlatformPackage*>(context);
    package->data_items = reply->data_items;
    package->fd_items = reply->fd_items;
    memcpy(package->data, reply->data, sizeof(package->data[0]) * package->data_items);
    memcpy(package->fd, reply->fd, sizeof(package->fd[0]) * package->fd_items);
}

struct ClientPlatformOperation : mtf::ConnectedClientHeadlessServer
{
    MirPlatformPackage platform_operation_add(int num1, int num2)
    {
        return platform_operation_add({num1,num2});
    }

    MirPlatformPackage incorrect_platform_operation_add()
    {
        return platform_operation_add({7});
    }

    MirPlatformPackage platform_operation_add(std::vector<int> const& nums)
    {
        unsigned int const add_opcode =
            static_cast<unsigned int>(mtf::StubGraphicsPlatformOperation::add);

        MirPlatformPackage request{0, 0, {}, {}};
        MirPlatformPackage reply{0, 0, {}, {}};
        request.data_items = nums.size();
        std::copy(nums.begin(), nums.end(), request.data);

        auto const platform_op_done = mir_connection_platform_operation(
            connection, add_opcode, &request, assign_package, &reply);
        mir_wait_for(platform_op_done);

        return reply;
    }
};

}

TEST_F(ClientPlatformOperation, exchanges_data_items_with_platform)
{
    using namespace testing;

    int const num1 = 7;
    int const num2 = 11;

    auto const reply = platform_operation_add(num1, num2);

    EXPECT_THAT(reply.fd_items, Eq(0));
    EXPECT_THAT(reply.data_items, Eq(1));
    EXPECT_THAT(reply.data[0], Eq(num1 + num2));
}

TEST_F(ClientPlatformOperation, does_not_set_connection_error_message_on_success)
{
    using namespace testing;

    platform_operation_add(7, 11);

    EXPECT_THAT(mir_connection_get_error_message(connection), StrEq(""));
}

TEST_F(ClientPlatformOperation, returns_empty_reply_on_error)
{
    using namespace testing;

    auto const reply = incorrect_platform_operation_add();

    EXPECT_THAT(reply.data_items, Eq(0));
    EXPECT_THAT(reply.fd_items, Eq(0));
}

TEST_F(ClientPlatformOperation, sets_connection_error_message_on_error)
{
    using namespace testing;

    incorrect_platform_operation_add();

    EXPECT_THAT(mir_connection_get_error_message(connection), StrNe(""));
}
