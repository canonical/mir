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
#include "src/platforms/mesa/server/kms/nested_authentication.h"
#include "mir_toolkit/mesa/platform_operation.h"
#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_nested_context.h"
#include "mir/test/fake_shared.h"
#include <gtest/gtest.h>

#include <cstring>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
struct NestedAuthentication : public ::testing::Test
{
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    mtd::MockNestedContext mock_nested_context;
    mgm::NestedAuthentication auth{mt::fake_shared(mock_nested_context)};
};
}

TEST_F(NestedAuthentication, uses_nested_context_for_auth_magic)
{
    using namespace testing;

    unsigned int const magic{332211};

    MirMesaAuthMagicRequest const request{magic};
    mg::PlatformOperationMessage msg;
    msg.data.resize(sizeof(request));
    std::memcpy(msg.data.data(), &request, sizeof(request));

    MirMesaAuthMagicResponse const success_response{0};
    mg::PlatformOperationMessage auth_magic_success_response;
    auth_magic_success_response.data.resize(sizeof(success_response));
    std::memcpy(auth_magic_success_response.data.data(), &success_response,
                sizeof(success_response));

    EXPECT_CALL(mock_nested_context,
                platform_operation(MirMesaPlatformOperation::auth_magic, msg))
        .WillOnce(Return(auth_magic_success_response));

    auth.auth_magic(magic);
}

TEST_F(NestedAuthentication, uses_nested_context_for_auth_fd)
{
    using namespace testing;

    mg::PlatformOperationMessage msg;

    int const auth_fd{13};
    mg::PlatformOperationMessage const response{{}, {auth_fd}};

    EXPECT_CALL(mock_nested_context,
                platform_operation(MirMesaPlatformOperation::auth_fd, msg))
        .WillOnce(Return(response));

    EXPECT_THAT(auth.authenticated_fd(), Eq(auth_fd));
}
