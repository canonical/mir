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
#include "mir/graphics/native_platform.h"
#include "mir/graphics/buffer_ipc_packer.h"
#include "mir/options/program_option.h"
#include "src/platform/graphics/android/android_platform.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_android_hw.h"
#include "mir_test_doubles/mock_buffer_packer.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/stub_display_builder.h"
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
        for(auto i=0u; i< (num_ints+num_fds); i++)
        {
            native_buffer_handle->data[i] = i;
        }

        native_buffer = std::make_shared<mtd::StubAndroidNativeBuffer>();
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
TEST_F(PlatformBufferIPCPackaging, test_ipc_data_packed_correctly)
{
    using namespace ::testing;

    mga::AndroidPlatform platform(stub_display_builder, stub_display_report);

    mtd::MockPacker mock_packer;
    int offset = 0;
    for(auto i=0u; i<num_fds; i++)
    {
        EXPECT_CALL(mock_packer, pack_fd(native_buffer_handle->data[offset++]))
            .Times(1);
    }
    for(auto i=0u; i<num_ints; i++)
    {
        EXPECT_CALL(mock_packer, pack_data(native_buffer_handle->data[offset++]))
            .Times(1);
    }

    EXPECT_CALL(*mock_buffer, stride())
        .WillOnce(Return(stride));
    EXPECT_CALL(mock_packer, pack_stride(stride))
        .Times(1);

    EXPECT_CALL(*mock_buffer, size())
        .WillOnce(Return(mir::geometry::Size{123, 456}));
    EXPECT_CALL(mock_packer, pack_size(_))
        .Times(1);

    platform.fill_ipc_package(&mock_packer, mock_buffer.get());
}

TEST(AndroidGraphicsPlatform, egl_native_display_is_egl_default_display)
{
    mga::AndroidPlatform platform(
        std::make_shared<mtd::StubDisplayBuilder>(),
        mr::null_display_report());

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

    auto platform = mg::create_native_platform(mt::fake_shared(stub_report));
}
