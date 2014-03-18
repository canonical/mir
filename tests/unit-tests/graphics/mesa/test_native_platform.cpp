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
#include "mir_test_doubles/mock_buffer_packer.h"

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

class StubBuffer : public mtd::StubBuffer
{
public:
    StubBuffer()
        : mtd::StubBuffer{
              mg::BufferProperties{
                  geom::Size{123, 456},
                  mir_pixel_format_abgr_8888,
                  mg::BufferUsage::software},
              geom::Stride{4390}},
          native_buffer{std::make_shared<mg::NativeBuffer>()}
    {
        native_buffer->data_items = 4;
        native_buffer->fd_items = 2;
        for(auto i = 0; i < native_buffer->data_items; i++)
            native_buffer->data[i] = i;
        for(auto i = 0; i < native_buffer->fd_items; i++)
            native_buffer->fd[i] = i;
        native_buffer->width = buf_size.width.as_int();
        native_buffer->height = buf_size.height.as_int();
        native_buffer->flags = 0x66;
        native_buffer->stride = buf_stride.as_int();
    }

    std::shared_ptr<mg::NativeBuffer> native_buffer_handle() const override
    {
        return native_buffer;
    }

private:
    std::shared_ptr<mg::NativeBuffer> native_buffer;
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

    mgm::NativePlatform native;

    EXPECT_CALL(mock_nested_context, drm_auth_magic(_));

    native.initialize(mt::fake_shared(mock_nested_context));
    native.get_ipc_package();
}

TEST_F(MesaNativePlatformTest, sets_gbm_device_during_initialization)
{
    using namespace testing;

    mgm::NativePlatform native;

    EXPECT_CALL(mock_nested_context, drm_set_gbm_device(mock_gbm.fake_gbm.device));

    native.initialize(mt::fake_shared(mock_nested_context));
}

TEST_F(MesaNativePlatformTest, packs_buffer_ipc_package_correctly)
{
    using namespace testing;

    StubBuffer const stub_buffer;
    mtd::MockPacker mock_packer;
    auto const native_buffer = stub_buffer.native_buffer_handle();

    for(auto i = 0; i < native_buffer->fd_items; i++)
        EXPECT_CALL(mock_packer, pack_fd(native_buffer->fd[i]));

    for(auto i = 0; i < native_buffer->data_items; i++)
        EXPECT_CALL(mock_packer, pack_data(native_buffer->data[i]));

    EXPECT_CALL(mock_packer, pack_stride(stub_buffer.stride()));
    EXPECT_CALL(mock_packer, pack_flags(native_buffer->flags));
    EXPECT_CALL(mock_packer, pack_size(stub_buffer.size()));

    mgm::NativePlatform native;

    native.fill_ipc_package(&mock_packer, &stub_buffer);
}
