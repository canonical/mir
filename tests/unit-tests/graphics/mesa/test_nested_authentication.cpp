/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/nested_context.h"
#include "mir/graphics/platform_operation_message.h"
#include "src/platforms/mesa/server/nested_authentication.h"
#include "mir_toolkit/mesa/platform_operation.h"
#include "mir_test_doubles/mock_drm.h"
#include "mir_test_doubles/mock_nested_context.h"
#include "mir_test/fake_shared.h"
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
struct NestedAuthentication : public ::testing::Test
{
    NestedAuthentication()
    {
        int const success{0};
        auth_magic_success_response.data.resize(sizeof(MirMesaAuthMagicResponse));
        *reinterpret_cast<MirMesaAuthMagicResponse*>(
            auth_magic_success_response.data.data()) =
                MirMesaAuthMagicResponse{success};
    }

    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    mtd::MockNestedContext mock_nested_context;
    mgm::NestedAuthentication auth{mt::fake_shared(mock_nested_context)};
    mg::PlatformOperationMessage auth_magic_success_response;
};
}

namespace mir
{
namespace graphics
{

bool operator==(mg::PlatformOperationMessage const& msg1,
                mg::PlatformOperationMessage const& msg2)
{
    return msg1.data == msg2.data &&
           msg1.fds == msg1.fds;
}
}

}

TEST_F(NestedAuthentication, uses_nested_context_to_authenticate)
{
    using namespace testing;

    unsigned int const magic{332211};

    mg::PlatformOperationMessage msg;
    msg.data.resize(sizeof(MirMesaAuthMagicRequest));
    *reinterpret_cast<MirMesaAuthMagicRequest*>(msg.data.data()) =
        MirMesaAuthMagicRequest{magic};

    EXPECT_CALL(mock_nested_context,
                platform_operation(MirMesaPlatformOperation::auth_magic, msg))
        .WillOnce(Return(auth_magic_success_response));

    auth.auth_magic(magic);
}

TEST_F(NestedAuthentication, no_drm_in_platform_package_throws)
{
    using namespace testing;
    EXPECT_CALL(mock_nested_context, platform_fd_items())
        .WillOnce(Return(std::vector<int>{}));
    EXPECT_THROW({
        auth.authenticated_fd();
    }, std::runtime_error);
}

TEST_F(NestedAuthentication, failure_in_getbusid_throws)
{
    using namespace testing;
    int stub_fd = 33;

    InSequence seq;
    EXPECT_CALL(mock_nested_context, platform_fd_items())
        .WillOnce(Return(std::vector<int>{stub_fd}));
    EXPECT_CALL(mock_drm, drmGetBusid(stub_fd))
        .WillOnce(Return(nullptr));

    EXPECT_THROW({
        auth.authenticated_fd();
    }, std::runtime_error);
}

TEST_F(NestedAuthentication, failure_in_getmagic_throws)
{
    using namespace testing;
    int stub_fd = 33;
    int auth_fd = 34;
    //this should get freed by the implementation
    auto fake_busid = static_cast<char*>(malloc(sizeof(char)));
    *fake_busid = 'a';
    int magic{332211};

    InSequence seq;
    EXPECT_CALL(mock_nested_context, platform_fd_items())
        .WillOnce(Return(std::vector<int>{stub_fd}));
    EXPECT_CALL(mock_drm, drmGetBusid(stub_fd))
        .WillOnce(Return(fake_busid));
    EXPECT_CALL(mock_drm, drmOpen(nullptr, fake_busid))
        .WillOnce(Return(auth_fd));
    EXPECT_CALL(mock_drm, drmGetMagic(auth_fd, _))
        .WillOnce(DoAll(SetArgPointee<1>(magic), Return(-1)));

    EXPECT_THROW({
        auth.authenticated_fd();
    }, std::runtime_error);
}

TEST_F(NestedAuthentication, creates_new_fd)
{
    using namespace testing;
    int stub_fd = 33;
    int auth_fd = 34;
    //this should get freed by the implementation
    auto fake_busid = static_cast<char*>(malloc(sizeof(char)));
    *fake_busid = 'a';
    int magic{332211};

    InSequence seq;
    EXPECT_CALL(mock_nested_context, platform_fd_items())
        .WillOnce(Return(std::vector<int>{stub_fd}));
    EXPECT_CALL(mock_drm, drmGetBusid(stub_fd))
        .WillOnce(Return(fake_busid));
    EXPECT_CALL(mock_drm, drmOpen(nullptr, fake_busid))
        .WillOnce(Return(auth_fd));
    EXPECT_CALL(mock_drm, drmGetMagic(auth_fd, _))
        .WillOnce(DoAll(SetArgPointee<1>(magic), Return(0)));
    EXPECT_CALL(mock_nested_context,
                platform_operation(MirMesaPlatformOperation::auth_magic, _))
        .WillOnce(Return(auth_magic_success_response));

    auto fd = auth.authenticated_fd();
    EXPECT_THAT(fd, Eq(auth_fd));
}
