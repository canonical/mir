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

#include "mir/graphics/null_display_report.h"
#include "mir/graphics/buffer_ipc_packer.h"
#include "mir/options/program_option.h"
#include "src/server/graphics/android/android_platform.h"
#include "src/server/graphics/android/display_buffer_factory.h"
#include "src/server/graphics/android/resource_factory.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_buffer_packer.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/mock_android_hw.h"
#include "mir_test_doubles/mock_fb_hal_device.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_android_native_buffer.h"
#include "mir_test_doubles/stub_display_device.h"
#include <system/window.h>
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;
namespace mo=mir::options;

class PlatformBufferIPCPackaging : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        stub_display_report = std::make_shared<mg::NullDisplayReport>();
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
        mock_buffer = std::make_shared<mtd::MockBuffer>();

        ON_CALL(*native_buffer, handle())
            .WillByDefault(Return(native_buffer_handle.get()));
        ON_CALL(*mock_buffer, native_buffer_handle())
            .WillByDefault(Return(native_buffer));
        ON_CALL(*mock_buffer, stride())
            .WillByDefault(Return(stride));
    }

    std::shared_ptr<mtd::MockAndroidNativeBuffer> native_buffer;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    std::shared_ptr<native_handle_t> native_buffer_handle;
    std::shared_ptr<mg::DisplayReport> stub_display_report;
    geom::Stride stride;
    unsigned int num_ints, num_fds;
};

/* ipc packaging tests */
TEST_F(PlatformBufferIPCPackaging, test_ipc_data_packed_correctly)
{
    auto platform = mg::create_platform(std::make_shared<mo::ProgramOption>(), stub_display_report);

    auto mock_packer = std::make_shared<mtd::MockPacker>();
    int offset = 0;
    for(auto i=0u; i<num_fds; i++)
    {
        EXPECT_CALL(*mock_packer, pack_fd(native_buffer_handle->data[offset++]))
            .Times(1);
    } 
    for(auto i=0u; i<num_ints; i++)
    {
        EXPECT_CALL(*mock_packer, pack_data(native_buffer_handle->data[offset++]))
            .Times(1);
    }
    EXPECT_CALL(*mock_packer, pack_stride(stride))
        .Times(1);

    platform->fill_ipc_package(mock_packer, mock_buffer);
}

namespace
{
struct MockResourceFactory: public mga::DisplayResourceFactory
{
    ~MockResourceFactory() noexcept {}
    MockResourceFactory()
    {
        using namespace testing;
        ON_CALL(*this, create_hwc_native_device()).WillByDefault(Return(nullptr));
        ON_CALL(*this, create_fb_native_device()).WillByDefault(Return(nullptr));
        ON_CALL(*this, create_native_window(_)).WillByDefault(Return(nullptr));
        ON_CALL(*this, create_fb_device(_)).WillByDefault(Return(nullptr));
        ON_CALL(*this, create_hwc11_device(_)).WillByDefault(Return(nullptr));
        ON_CALL(*this, create_hwc10_device(_,_)).WillByDefault(Return(nullptr));
    }

    MOCK_CONST_METHOD0(create_hwc_native_device, std::shared_ptr<hwc_composer_device_1>());
    MOCK_CONST_METHOD0(create_fb_native_device, std::shared_ptr<framebuffer_device_t>());

    MOCK_CONST_METHOD1(create_native_window,
        std::shared_ptr<ANativeWindow>(std::shared_ptr<mga::DisplayDevice> const&));

    MOCK_CONST_METHOD1(create_fb_device,
        std::shared_ptr<mga::DisplayDevice>(std::shared_ptr<framebuffer_device_t> const&));
    MOCK_CONST_METHOD1(create_hwc11_device,
        std::shared_ptr<mga::DisplayDevice>(std::shared_ptr<hwc_composer_device_1> const&));
    MOCK_CONST_METHOD2(create_hwc10_device,
        std::shared_ptr<mga::DisplayDevice>(
            std::shared_ptr<hwc_composer_device_1> const&, std::shared_ptr<framebuffer_device_t> const&));
};

class DisplayBufferCreation : public ::testing::Test
{
public:
    void SetUp()
    {
        using namespace testing;
        int fb_format = HAL_PIXEL_FORMAT_RGBA_8888;
        mock_resource_factory = std::make_shared<testing::NiceMock<MockResourceFactory>>();

        mock_display_report = std::make_shared<mtd::MockDisplayReport>();
        fb_access_mock = std::make_shared<testing::NiceMock<mtd::MockFBHalDevice>>(
            1, 1, fb_format, 2);

        ON_CALL(*mock_resource_factory, create_hwc_native_device())
            .WillByDefault(Return(hw_access_mock.mock_hwc_device));
        ON_CALL(*mock_resource_factory, create_fb_native_device())
            .WillByDefault(Return(fb_access_mock));
        ON_CALL(mock_egl, eglGetConfigAttrib(_,_,_,_))
            .WillByDefault(DoAll(SetArgPointee<3>(fb_format), Return(EGL_TRUE)));
    }

    testing::NiceMock<mtd::MockEGL> mock_egl;
    testing::NiceMock<mtd::HardwareAccessMock> hw_access_mock;
    std::shared_ptr<mtd::MockFBHalDevice> fb_access_mock;
    std::shared_ptr<MockResourceFactory> mock_resource_factory;
    std::shared_ptr<mtd::MockDisplayReport> mock_display_report;
};
}


//HWC 1.0
TEST_F(DisplayBufferCreation, hwc_version_10_success)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_0;

    EXPECT_CALL(*mock_resource_factory, create_hwc_native_device())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_fb_native_device())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_hwc10_device(_,_))
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_hwc_composition_in_use(1,0))
        .Times(1);

    mga::DisplayBufferFactory factory(mock_resource_factory, mock_display_report);
    factory.create_display_device();
}

TEST_F(DisplayBufferCreation, hwc_version_10_failure_uses_gpu)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_0;

    EXPECT_CALL(*mock_resource_factory, create_hwc_native_device())
        .Times(1)
        .WillOnce(Throw(std::runtime_error("")));
    EXPECT_CALL(*mock_resource_factory, create_fb_native_device())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_fb_device(_))
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_gpu_composition_in_use())
        .Times(1);

    mga::DisplayBufferFactory factory(mock_resource_factory, mock_display_report);
    factory.create_display_device();
}

//HWC 1.0
TEST_F(DisplayBufferCreation, hwc_version_11_egl_selection)
{
    using namespace testing;
    EGLint const default_egl_config_attr [] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_FRAMEBUFFER_TARGET_ANDROID, EGL_TRUE,
        EGL_NONE
    };

    EGLint const backup_egl_config_attr [] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig fake_egl_config = reinterpret_cast<EGLConfig>(0x44);

    Sequence seq;
    EXPECT_CALL(mock_egl, eglChooseConfig(_,mtd::AttrMatches(default_egl_config_attr),_,_,_))
        .InSequence(seq)
        .WillOnce(DoAll(SetArgPointee<4>(0), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglChooseConfig(_,mtd::AttrMatches(backup_egl_config_attr),_,_,_))
        .InSequence(seq)
        .WillOnce(DoAll(SetArgPointee<2>(fake_egl_config), SetArgPointee<4>(1), Return(EGL_TRUE)));

    mga::DisplayBufferFactory factory(mock_resource_factory, mock_display_report);
    EXPECT_EQ(fake_egl_config, factory.egl_config());
}

TEST_F(DisplayBufferCreation, hwc_version_11_success)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_1;

    EXPECT_CALL(*mock_resource_factory, create_hwc_native_device())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_hwc11_device(_))
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_hwc_composition_in_use(1,1))
        .Times(1);

    mga::DisplayBufferFactory factory(mock_resource_factory, mock_display_report);
    factory.create_display_device();
}

TEST_F(DisplayBufferCreation, hwc_version_11_hwc_failure)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_1;

    EXPECT_CALL(*mock_resource_factory, create_hwc_native_device())
        .Times(1)
        .WillOnce(Throw(std::runtime_error("")));
    EXPECT_CALL(*mock_resource_factory, create_fb_native_device())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_fb_device(_))
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_gpu_composition_in_use())
        .Times(1);

    mga::DisplayBufferFactory factory(mock_resource_factory, mock_display_report);
    factory.create_display_device();
}

TEST_F(DisplayBufferCreation, hwc_version_11_hwc_and_fb_failure_fatal)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_1;

    EXPECT_CALL(*mock_resource_factory, create_hwc_native_device())
        .Times(1)
        .WillOnce(Throw(std::runtime_error("")));
    EXPECT_CALL(*mock_resource_factory, create_fb_native_device())
        .Times(1)
        .WillOnce(Throw(std::runtime_error("")));

    EXPECT_THROW({
        mga::DisplayBufferFactory factory(mock_resource_factory, mock_display_report);
    }, std::runtime_error);
}

//we don't support hwc 1.2 quite yet. for the time being, at least try the fb backup
TEST_F(DisplayBufferCreation, hwc_version_12_attempts_fb_backup)
{
    using namespace testing;

    hw_access_mock.mock_hwc_device->common.version = HWC_DEVICE_API_VERSION_1_2;

    EXPECT_CALL(*mock_resource_factory, create_hwc_native_device())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_fb_native_device())
        .Times(1);
    EXPECT_CALL(*mock_resource_factory, create_fb_device(_))
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_gpu_composition_in_use())
        .Times(1);

    mga::DisplayBufferFactory factory(mock_resource_factory, mock_display_report);
    factory.create_display_device();
}

TEST_F(DisplayBufferCreation, db_creation)
{
    using namespace testing;
    mtd::StubDisplayDevice stub_device;
    EXPECT_CALL(*mock_resource_factory, create_native_window(_))
        .Times(1);

    mga::DisplayBufferFactory factory(mock_resource_factory, mock_display_report);
    factory.create_display_buffer(mt::fake_shared(stub_device));
}
