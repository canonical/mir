/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/nested_context.h"
#include "src/platforms/mesa/server/kms/guest_platform.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir_toolkit/mesa/platform_operation.h"

#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_gbm.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/mock_buffer_ipc_message.h"
#include "mir/test/doubles/fd_matcher.h"
#include "mir/test/doubles/mock_nested_context.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstring>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;

namespace
{

class MesaGuestPlatformTest : public ::testing::Test
{
public:
    MesaGuestPlatformTest()
    {
        using namespace testing;

        MirMesaSetGBMDeviceResponse const response_success{0};
        mg::PlatformOperationMessage set_gbm_device_success_msg;
        set_gbm_device_success_msg.data.resize(sizeof(response_success));
        std::memcpy(set_gbm_device_success_msg.data.data(),
                    &response_success, sizeof(response_success));

        ON_CALL(mock_nested_context, platform_fd_items())
            .WillByDefault(Return(std::vector<int>{mock_drm.fake_drm.fd()}));
        ON_CALL(mock_nested_context,
                platform_operation(MirMesaPlatformOperation::set_gbm_device, _))
            .WillByDefault(Return(set_gbm_device_success_msg));
    }

protected:
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    ::testing::NiceMock<mtd::MockNestedContext> mock_nested_context;
};

}

TEST_F(MesaGuestPlatformTest, auth_fd_is_delegated_to_nested_context)
{
    using namespace testing;

    int const auth_fd{13};
    mg::PlatformOperationMessage auth_fd_response{{},{auth_fd}};

    EXPECT_CALL(mock_nested_context,
                platform_operation(MirMesaPlatformOperation::set_gbm_device, _));
    EXPECT_CALL(mock_nested_context,
                platform_operation(MirMesaPlatformOperation::auth_fd, _))
        .WillOnce(Return(auth_fd_response));

    mgm::GuestPlatform native(mt::fake_shared(mock_nested_context));
    auto ipc_ops = native.make_ipc_operations();
    ipc_ops->connection_ipc_package();
}

TEST_F(MesaGuestPlatformTest, sets_gbm_device_during_initialization)
{
    MirMesaSetGBMDeviceRequest const request{mock_gbm.fake_gbm.device};
    mg::PlatformOperationMessage request_msg;
    request_msg.data.resize(sizeof(request));
    std::memcpy(request_msg.data.data(), &request, sizeof(request));

    EXPECT_CALL(mock_nested_context,
                platform_operation(MirMesaPlatformOperation::set_gbm_device,
                                   request_msg));

    mgm::GuestPlatform native(mt::fake_shared(mock_nested_context));
}
