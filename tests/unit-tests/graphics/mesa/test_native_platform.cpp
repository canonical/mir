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
#include "src/platform/graphics/mesa/native_platform.h"
#include "mir/graphics/buffer_properties.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_drm.h"
#include "mir_test_doubles/mock_gbm.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/mock_buffer_ipc_message.h"
#include "mir_test_doubles/fd_matcher.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;

namespace
{

struct MockNestedContext : public mg::NestedContext
{
    MOCK_METHOD0(platform_fd_items, std::vector<int>());
    MOCK_METHOD1(drm_auth_magic, void(int));
    MOCK_METHOD1(drm_set_gbm_device, void(struct gbm_device*));
};

class MesaNativePlatformTest : public ::testing::Test
{
public:
    MesaNativePlatformTest()
    {
        using namespace testing;

        ON_CALL(mock_nested_context, platform_fd_items())
            .WillByDefault(Return(std::vector<int>{mock_drm.fake_drm.fd()}));
    }

protected:
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    ::testing::NiceMock<MockNestedContext> mock_nested_context;
};

}

TEST_F(MesaNativePlatformTest, auth_magic_is_delegated_to_nested_context)
{
    using namespace testing;
    EXPECT_CALL(mock_nested_context, drm_auth_magic(_));

    mgm::NativePlatform native(mt::fake_shared(mock_nested_context));
    native.connection_ipc_package();
}

TEST_F(MesaNativePlatformTest, sets_gbm_device_during_initialization)
{
    EXPECT_CALL(mock_nested_context, drm_set_gbm_device(mock_gbm.fake_gbm.device));
    mgm::NativePlatform native(mt::fake_shared(mock_nested_context));
}

TEST_F(MesaNativePlatformTest, packs_buffer_ipc_package_correctly)
{
    using namespace testing;

    int const width{123};
    int const height{456};
    geom::Size size{width, height};
    auto stub_native_buffer = std::make_shared<mtd::StubGBMNativeBuffer>(size);
    mg::BufferProperties properties{size, mir_pixel_format_abgr_8888, mg::BufferUsage::software};
    mtd::StubBuffer const stub_buffer(
        stub_native_buffer, properties, geom::Stride{stub_native_buffer->stride});
    mtd::MockBufferIpcMessage mock_ipc_msg;
    auto const native_buffer = stub_buffer.native_buffer_handle();

    for(auto i = 0; i < native_buffer->fd_items; i++)
        EXPECT_CALL(mock_ipc_msg, pack_fd(mtd::RawFdMatcher(native_buffer->fd[i])))
            .Times(Exactly(1));

    for(auto i = 0; i < native_buffer->data_items; i++)
        EXPECT_CALL(mock_ipc_msg, pack_data(native_buffer->data[i]))
            .Times(Exactly(1));

    EXPECT_CALL(mock_ipc_msg, pack_stride(stub_buffer.stride()))
        .Times(Exactly(1));
    EXPECT_CALL(mock_ipc_msg, pack_flags(native_buffer->flags))
        .Times(Exactly(1));
    EXPECT_CALL(mock_ipc_msg, pack_size(stub_buffer.size()))
        .Times(Exactly(1));

    mgm::NativePlatform native(mt::fake_shared(mock_nested_context));

    native.fill_buffer_package(&mock_ipc_msg, &stub_buffer, mg::BufferIpcMsgType::full_msg);
    native.fill_buffer_package(&mock_ipc_msg, &stub_buffer, mg::BufferIpcMsgType::update_msg);
}
