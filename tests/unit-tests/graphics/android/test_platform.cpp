/*
 * Copyright © 2012 Canonical Ltd.
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

#include "src/server/report/null_report_factory.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/options/program_option.h"
#include "src/platforms/android/platform.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_android_hw.h"
#include "mir_test_doubles/mock_buffer_ipc_message.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/stub_display_builder.h"
#include "mir_test_doubles/fd_matcher.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_android_native_buffer.h"
#include <system/window.h>
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace mr=mir::report;
namespace geom=mir::geometry;
namespace mo=mir::options;

class PlatformBufferIPCPackaging : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        stub_display_builder = std::make_shared<mtd::StubDisplayBuilder>();
        stub_display_report = mr::null_display_report();
        stride = geom::Stride(300*4);

        num_ints = 43;
        num_fds = 55;
        auto handle_size = sizeof(native_handle_t) + (sizeof(int)*(num_ints + num_fds));
        auto native_buffer_raw = (native_handle_t*) ::operator new(handle_size);
        native_buffer_handle = std::shared_ptr<native_handle_t>(native_buffer_raw);

        native_buffer_handle->numInts = num_ints;
        native_buffer_handle->numFds = num_fds;
        for (auto i = 0u; i < (num_ints+num_fds); i++)
        {
            native_buffer_handle->data[i] = i;
        }

        native_buffer = std::make_shared<mtd::MockAndroidNativeBuffer>();
        mock_buffer = std::make_shared<NiceMock<mtd::MockBuffer>>();

        ON_CALL(*native_buffer, handle())
            .WillByDefault(Return(native_buffer_handle.get()));
        ON_CALL(*mock_buffer, native_buffer_handle())
            .WillByDefault(Return(native_buffer));
        ON_CALL(*mock_buffer, stride())
            .WillByDefault(Return(stride));
    }

    std::shared_ptr<mtd::MockAndroidNativeBuffer> native_buffer;
    std::shared_ptr<mtd::StubDisplayBuilder> stub_display_builder;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    std::shared_ptr<native_handle_t> native_buffer_handle;
    std::shared_ptr<mg::DisplayReport> stub_display_report;
    geom::Stride stride;
    unsigned int num_ints, num_fds;
};

/* ipc packaging tests */
TEST_F(PlatformBufferIPCPackaging, test_ipc_data_packed_correctly_for_full_ipc_with_fence)
{
    using namespace ::testing;
    int fake_fence{333};
    EXPECT_CALL(*native_buffer, copy_fence())
        .WillOnce(Return(fake_fence));

    mga::Platform platform(stub_display_builder, stub_display_report, mga::OverlayOptimization::enabled);

    mtd::MockBufferIpcMessage mock_ipc_msg;
    int offset = 0;
    EXPECT_CALL(mock_ipc_msg, pack_data(static_cast<int>(mga::BufferFlag::fenced)));
    EXPECT_CALL(mock_ipc_msg, pack_fd(mtd::RawFdMatcher(fake_fence)));
    for (auto i = 0u; i < num_fds; i++)
        EXPECT_CALL(mock_ipc_msg, pack_fd(mtd::RawFdMatcher(native_buffer_handle->data[offset++])));
    for (auto i = 0u; i < num_ints; i++)
        EXPECT_CALL(mock_ipc_msg, pack_data(native_buffer_handle->data[offset++]));

    EXPECT_CALL(*mock_buffer, stride())
        .WillOnce(Return(stride));
    EXPECT_CALL(mock_ipc_msg, pack_stride(stride))
        .Times(1);

    EXPECT_CALL(*mock_buffer, size())
        .WillOnce(Return(mir::geometry::Size{123, 456}));
    EXPECT_CALL(mock_ipc_msg, pack_size(_))
        .Times(1);

    auto ipc_ops = platform.make_ipc_operations();
    ipc_ops->pack_buffer(mock_ipc_msg, *mock_buffer, mg::BufferIpcMsgType::full_msg);
}

TEST_F(PlatformBufferIPCPackaging, test_ipc_data_packed_correctly_for_full_ipc_without_fence)
{
    using namespace ::testing;
    EXPECT_CALL(*native_buffer, copy_fence())
        .WillOnce(Return(-1));

    mga::Platform platform(stub_display_builder, stub_display_report, mga::OverlayOptimization::enabled);

    mtd::MockBufferIpcMessage mock_ipc_msg;
    int offset = 0;
    EXPECT_CALL(mock_ipc_msg, pack_data(static_cast<int>(mga::BufferFlag::unfenced)));
    EXPECT_CALL(mock_ipc_msg, pack_fd(mtd::RawFdMatcher(-1)))
        .Times(0);

    for (auto i = 0u; i < num_fds; i++)
    {
        EXPECT_CALL(mock_ipc_msg, pack_fd(mtd::RawFdMatcher(native_buffer_handle->data[offset++])))
            .Times(1);
    }
    for (auto i = 0u; i < num_ints; i++)
    {
        EXPECT_CALL(mock_ipc_msg, pack_data(native_buffer_handle->data[offset++]))
            .Times(1);
    }

    EXPECT_CALL(*mock_buffer, stride())
        .WillOnce(Return(stride));
    EXPECT_CALL(mock_ipc_msg, pack_stride(stride))
        .Times(1);

    EXPECT_CALL(*mock_buffer, size())
        .WillOnce(Return(mir::geometry::Size{123, 456}));
    EXPECT_CALL(mock_ipc_msg, pack_size(_))
        .Times(1);

    auto ipc_ops = platform.make_ipc_operations();
    ipc_ops->pack_buffer(mock_ipc_msg, *mock_buffer, mg::BufferIpcMsgType::full_msg);
}

TEST_F(PlatformBufferIPCPackaging, test_ipc_data_packed_correctly_for_nested)
{
    using namespace ::testing;
    EXPECT_CALL(*native_buffer, copy_fence())
        .WillOnce(Return(-1));

    mga::Platform platform(stub_display_builder, stub_display_report, mga::OverlayOptimization::enabled);

    mtd::MockBufferIpcMessage mock_ipc_msg;
    int offset = 0;
    for (auto i = 0u; i < num_fds; i++)
    {
        EXPECT_CALL(mock_ipc_msg, pack_fd(mtd::RawFdMatcher(native_buffer_handle->data[offset++])))
            .Times(1);
    }
    EXPECT_CALL(mock_ipc_msg, pack_data(static_cast<int>(mga::BufferFlag::unfenced)));
    for (auto i = 0u; i < num_ints; i++)
    {
        EXPECT_CALL(mock_ipc_msg, pack_data(native_buffer_handle->data[offset++]))
            .Times(1);
    }

    EXPECT_CALL(*mock_buffer, stride())
        .WillOnce(Return(stride));
    EXPECT_CALL(mock_ipc_msg, pack_stride(stride))
        .Times(1);

    EXPECT_CALL(*mock_buffer, size())
        .WillOnce(Return(mir::geometry::Size{123, 456}));
    EXPECT_CALL(mock_ipc_msg, pack_size(_))
        .Times(1);

    auto ipc_ops = platform.make_ipc_operations();
    ipc_ops->pack_buffer(mock_ipc_msg, *mock_buffer, mg::BufferIpcMsgType::full_msg);
}

TEST_F(PlatformBufferIPCPackaging, test_ipc_data_packed_correctly_for_partial_ipc)
{
    using namespace ::testing;

    int fake_fence{33};
    mga::Platform platform(stub_display_builder, stub_display_report, mga::OverlayOptimization::enabled);
    auto ipc_ops = platform.make_ipc_operations();

    mtd::MockBufferIpcMessage mock_ipc_msg;

    Sequence seq;
    EXPECT_CALL(mock_ipc_msg, pack_data(static_cast<int>(mga::BufferFlag::fenced)))
        .InSequence(seq);
    EXPECT_CALL(mock_ipc_msg, pack_fd(mtd::RawFdMatcher(fake_fence)))
        .InSequence(seq);
    EXPECT_CALL(mock_ipc_msg, pack_data(static_cast<int>(mga::BufferFlag::unfenced)))
        .InSequence(seq);

    EXPECT_CALL(*native_buffer, copy_fence())
        .Times(2)
        .WillOnce(Return(fake_fence))
        .WillOnce(Return(-1));

    ipc_ops->pack_buffer(mock_ipc_msg, *mock_buffer, mg::BufferIpcMsgType::update_msg);
    ipc_ops->pack_buffer(mock_ipc_msg, *mock_buffer, mg::BufferIpcMsgType::update_msg);
}

TEST(AndroidGraphicsPlatform, egl_native_display_is_egl_default_display)
{
    mga::Platform platform(
        std::make_shared<mtd::StubDisplayBuilder>(),
        mr::null_display_report(),
        mga::OverlayOptimization::enabled);
    EXPECT_EQ(EGL_DEFAULT_DISPLAY, platform.egl_native_display());
}

TEST(NestedPlatformCreation, doesnt_access_display_hardware)
{
    using namespace testing;

    mtd::HardwareAccessMock hwaccess;
    mtd::MockDisplayReport stub_report;

    EXPECT_CALL(hwaccess, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID), _))
        .Times(0);
    EXPECT_CALL(hwaccess, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID), _))
        .Times(AtMost(1));

    auto platform = mg::create_guest_platform(mt::fake_shared(stub_report), nullptr);
}
