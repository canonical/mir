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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */
#include <boost/throw_exception.hpp>
#include "src/server/graphics/gbm/gbm_platform.h"
#include "src/server/graphics/gbm/gbm_display.h"
#include "src/server/graphics/gbm/virtual_terminal.h"
#include "src/server/logging/display_report.h"
#include "mir/logging/logger.h"
#include "mir/graphics/display_buffer.h"
#include "src/server/graphics/default_display_configuration_policy.h"
#include "mir/asio_main_loop.h"

#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_gl.h"
#include "mir/graphics/null_display_report.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/null_virtual_terminal.h"
#include "mir_test_doubles/null_video_devices.h"

#include "mir_test_doubles/mock_drm.h"
#include "mir_test_doubles/mock_gbm.h"

#include "mir_test_framework/udev_environment.h"

#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mgg=mir::graphics::gbm;
namespace ml=mir::logging;
namespace mtd=mir::test::doubles;
namespace mtf=mir::mir_test_framework;

namespace
{
struct MockLogger : public ml::Logger
{
    MOCK_METHOD3(log,
                 void(ml::Logger::Severity, const std::string&, const std::string&));

    ~MockLogger() noexcept(true) {}
};

class MockVirtualTerminal : public mgg::VirtualTerminal
{
public:
    ~MockVirtualTerminal() noexcept(true) {}

    MOCK_METHOD0(set_graphics_mode, void());
    MOCK_METHOD3(register_switch_handlers,
                 void(mg::EventHandlerRegister&,
                      std::function<bool()> const&,
                      std::function<bool()> const&));
};

class MockVideoDevices : public mgg::VideoDevices
{
public:
    ~MockVideoDevices() noexcept(true) {}

    MOCK_METHOD2(register_change_handler,
                 void(mg::EventHandlerRegister&,
                      std::function<void()> const&));
};

class GBMDisplayTest : public ::testing::Test
{
public:
    GBMDisplayTest() :
        mock_report{std::make_shared<testing::NiceMock<mtd::MockDisplayReport>>()},
        null_report{std::make_shared<mg::NullDisplayReport>()}
    {
        using namespace testing;
        ON_CALL(mock_egl, eglChooseConfig(_,_,_,1,_))
        .WillByDefault(DoAll(SetArgPointee<2>(mock_egl.fake_configs[0]),
                             SetArgPointee<4>(1),
                             Return(EGL_TRUE)));

        const char* egl_exts = "EGL_KHR_image EGL_KHR_image_base EGL_MESA_drm_image";
        const char* gl_exts = "GL_OES_texture_npot GL_OES_EGL_image";

        ON_CALL(mock_egl, eglQueryString(_,EGL_EXTENSIONS))
        .WillByDefault(Return(egl_exts));
        ON_CALL(mock_gl, glGetString(GL_EXTENSIONS))
        .WillByDefault(Return(reinterpret_cast<const GLubyte*>(gl_exts)));

        /*
         * Silence uninteresting calls called when cleaning up resources in
         * the MockGBM destructor, and which are not handled by NiceMock<>.
         */
        EXPECT_CALL(mock_gbm, gbm_bo_get_device(_))
        .Times(AtLeast(0));
        EXPECT_CALL(mock_gbm, gbm_device_get_fd(_))
        .Times(AtLeast(0));

        fake_devices.add_standard_drm_devices();
    }

    std::shared_ptr<mgg::GBMPlatform> create_platform()
    {
        return std::make_shared<mgg::GBMPlatform>(
            null_report,
            std::make_shared<mtd::NullVirtualTerminal>());
    }

    std::shared_ptr<mgg::GBMDisplay> create_display(
        std::shared_ptr<mgg::GBMPlatform> const& platform)
    {
        return std::make_shared<mgg::GBMDisplay>(
            platform,
            std::make_shared<mtd::NullVideoDevices>(),
            std::make_shared<mg::DefaultDisplayConfigurationPolicy>(),
            null_report);
    }

    void setup_post_update_expectations()
    {
        using namespace testing;

        EXPECT_CALL(mock_egl, eglSwapBuffers(mock_egl.fake_egl_display,
                                             mock_egl.fake_egl_surface))
            .Times(Exactly(2));

        EXPECT_CALL(mock_gbm, gbm_surface_lock_front_buffer(mock_gbm.fake_gbm.surface))
            .Times(Exactly(2))
            .WillOnce(Return(fake.bo1))
            .WillOnce(Return(fake.bo2));

        EXPECT_CALL(mock_gbm, gbm_bo_get_handle(fake.bo1))
            .Times(Exactly(1))
            .WillOnce(Return(fake.bo_handle1));

        EXPECT_CALL(mock_gbm, gbm_bo_get_handle(fake.bo2))
            .Times(Exactly(1))
            .WillOnce(Return(fake.bo_handle2));

        EXPECT_CALL(mock_drm, drmModeAddFB(mock_drm.fake_drm.fd(),
                                           _, _, _, _, _,
                                           fake.bo_handle1.u32, _))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<7>(fake.fb_id1), Return(0)));

        EXPECT_CALL(mock_drm, drmModeAddFB(mock_drm.fake_drm.fd(),
                                           _, _, _, _, _,
                                           fake.bo_handle2.u32, _))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<7>(fake.fb_id2), Return(0)));
    }

    uint32_t get_connected_connector_id()
    {
        auto drm_res = mock_drm.fake_drm.resources_ptr();

        for (int i = 0; i < drm_res->count_connectors; i++)
        {
            auto connector = mock_drm.fake_drm.find_connector(drm_res->connectors[i]);
            if (connector->connection == DRM_MODE_CONNECTED)
                return connector->connector_id;
        }

        return 0;
    }

    uint32_t get_connected_crtc_id()
    {
        auto connector_id = get_connected_connector_id();
        auto connector = mock_drm.fake_drm.find_connector(connector_id);

        if (connector)
        {
            auto encoder = mock_drm.fake_drm.find_encoder(connector->encoder_id);
            if (encoder)
                return encoder->crtc_id;
        }

        return 0;
    }

    struct FakeData {
        FakeData()
            : bo1{reinterpret_cast<gbm_bo*>(0xabcd)},
              bo2{reinterpret_cast<gbm_bo*>(0xabce)},
              fb_id1{66}, fb_id2{67}, crtc()
        {
            bo_handle1.u32 = 0x1234;
            bo_handle2.u32 = 0x1235;
            crtc.buffer_id = 88;
            crtc.crtc_id = 565;
        }

        gbm_bo* bo1;
        gbm_bo* bo2;
        uint32_t fb_id1;
        uint32_t fb_id2;
        gbm_bo_handle bo_handle1;
        gbm_bo_handle bo_handle2;
        drmModeCrtc crtc;
    } fake;

    ::testing::NiceMock<mtd::MockEGL> mock_egl;
    ::testing::NiceMock<mtd::MockGL> mock_gl;
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    std::shared_ptr<testing::NiceMock<mtd::MockDisplayReport>> const mock_report;
    std::shared_ptr<mg::DisplayReport> const null_report;
    mtf::UdevEnvironment fake_devices;
};

}

TEST_F(GBMDisplayTest, create_display)
{
    using namespace testing;

    auto const connector_id = get_connected_connector_id();
    auto const crtc_id = get_connected_crtc_id();

    /* To display a gbm surface, the GBMDisplay should... */

    /* Create a gbm surface to use as the frame buffer */
    EXPECT_CALL(mock_gbm, gbm_surface_create(mock_gbm.fake_gbm.device,_,_,_,_))
        .Times(Exactly(1));

    /* Create an EGL window surface backed by the gbm surface */
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display,
                                                 mock_egl.fake_configs[0],
                                                 (EGLNativeWindowType)mock_gbm.fake_gbm.surface, _))
        .Times(Exactly(1));

    /* Swap the EGL window surface to bring the back buffer to the front */
    EXPECT_CALL(mock_egl, eglSwapBuffers(mock_egl.fake_egl_display,
                                         mock_egl.fake_egl_surface))
        .Times(Exactly(1));

    /* Get the gbm_bo object corresponding to the front buffer */
    EXPECT_CALL(mock_gbm, gbm_surface_lock_front_buffer(mock_gbm.fake_gbm.surface))
        .Times(Exactly(1))
        .WillOnce(Return(fake.bo1));

    /* Get the DRM buffer handle associated with the gbm_bo */
    EXPECT_CALL(mock_gbm, gbm_bo_get_handle(fake.bo1))
        .Times(Exactly(1))
        .WillOnce(Return(fake.bo_handle1));

    /* Create a a DRM FB with the DRM buffer attached */
    EXPECT_CALL(mock_drm, drmModeAddFB(mock_drm.fake_drm.fd(),
                                       _, _, _, _, _,
                                       fake.bo_handle1.u32, _))
        .Times(Exactly(1))
        .WillOnce(DoAll(SetArgPointee<7>(fake.fb_id1), Return(0)));

    /* Display the DRM FB (first expectation is for cleanup) */
    EXPECT_CALL(mock_drm, drmModeSetCrtc(mock_drm.fake_drm.fd(),
                                         crtc_id, Ne(fake.fb_id1),
                                         _, _,
                                         Pointee(connector_id),
                                         _, _))
        .Times(AtLeast(0));

    EXPECT_CALL(mock_drm, drmModeSetCrtc(mock_drm.fake_drm.fd(),
                                         crtc_id, fake.fb_id1,
                                         _, _,
                                         Pointee(connector_id),
                                         _, _))
        .Times(Exactly(1))
        .WillOnce(Return(0));


    auto display = create_display(create_platform());
}

TEST_F(GBMDisplayTest, reset_crtc_on_destruction)
{
    using namespace testing;

    auto const connector_id = get_connected_connector_id();
    auto const crtc_id = get_connected_crtc_id();
    uint32_t const fb_id{66};

    /* Create DRM FBs */
    EXPECT_CALL(mock_drm, drmModeAddFB(mock_drm.fake_drm.fd(),
                                       _, _, _, _, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<7>(fb_id), Return(0)));


    {
        InSequence s;

        /* crtc is set */
        EXPECT_CALL(mock_drm, drmModeSetCrtc(mock_drm.fake_drm.fd(),
                                             crtc_id, fb_id,
                                             _, _,
                                             Pointee(connector_id),
                                             _, _))
            .Times(AtLeast(1));

        /* crtc is reset */
        EXPECT_CALL(mock_drm, drmModeSetCrtc(mock_drm.fake_drm.fd(),
                                             crtc_id, Ne(fb_id),
                                             _, _,
                                             Pointee(connector_id),
                                             _, _))
            .Times(1);
    }

    auto display = create_display(create_platform());
}

TEST_F(GBMDisplayTest, create_display_drm_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_drm, drmOpen(_,_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(-1));

    EXPECT_CALL(mock_drm, drmClose(_))
        .Times(Exactly(0));

    EXPECT_THROW(
    {
        auto display = create_display(create_platform());
    }, std::runtime_error);
}

TEST_F(GBMDisplayTest, create_display_kms_failure)
{
    using namespace testing;

    auto platform = create_platform();

    Mock::VerifyAndClearExpectations(&mock_drm);

    EXPECT_CALL(mock_drm, drmModeGetResources(_))
        .Times(Exactly(1))
        .WillOnce(Return(reinterpret_cast<drmModeRes*>(0)));

    EXPECT_CALL(mock_drm, drmModeFreeResources(_))
        .Times(Exactly(0));

    EXPECT_CALL(mock_drm, drmClose(_))
        .Times(Exactly(1));

    EXPECT_THROW({
        auto display = create_display(platform);
    }, std::runtime_error) << "Expected that c'tor of GBMDisplay throws";
}

TEST_F(GBMDisplayTest, create_display_gbm_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_create_device(_))
        .Times(Exactly(1))
        .WillOnce(Return(reinterpret_cast<gbm_device*>(0)));

    EXPECT_CALL(mock_gbm, gbm_device_destroy(_))
        .Times(Exactly(0));

    EXPECT_CALL(mock_drm, drmClose(_))
        .Times(Exactly(1));

    EXPECT_THROW({
        auto platform = create_platform();
    }, std::runtime_error) << "Expected c'tor of GBMPlatform to throw an exception";
}

namespace
{

ACTION_P(QueuePageFlipEvent, write_drm_fd)
{
    EXPECT_EQ(1, write(write_drm_fd, "a", 1));
}

ACTION_P(InvokePageFlipHandler, param)
{
    int const dont_care{0};
    char dummy;

    arg1->page_flip_handler(dont_care, dont_care, dont_care, dont_care, *param);
    ASSERT_EQ(1, read(arg0, &dummy, 1));
}

}

TEST_F(GBMDisplayTest, post_update)
{
    using namespace testing;

    auto const crtc_id = get_connected_crtc_id();
    void* user_data{nullptr};

    setup_post_update_expectations();

    {
        InSequence s;

        /* Flip the new FB */
        EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(),
                                              crtc_id,
                                              fake.fb_id2,
                                              _, _))
            .Times(Exactly(1))
            .WillOnce(DoAll(QueuePageFlipEvent(mock_drm.fake_drm.write_fd()),
                            SaveArg<4>(&user_data),
                            Return(0)));

        /* Handle the flip event */
        EXPECT_CALL(mock_drm, drmHandleEvent(mock_drm.fake_drm.fd(), _))
            .Times(1)
            .WillOnce(DoAll(InvokePageFlipHandler(&user_data), Return(0)));

        /* Release the current FB */
        EXPECT_CALL(mock_gbm, gbm_surface_release_buffer(mock_gbm.fake_gbm.surface, fake.bo1))
            .Times(Exactly(1));

        /* Release the new FB (at destruction time) */
        EXPECT_CALL(mock_gbm, gbm_surface_release_buffer(mock_gbm.fake_gbm.surface, fake.bo2))
            .Times(Exactly(1));
    }


    auto display = create_display(create_platform());

    display->for_each_display_buffer([](mg::DisplayBuffer& db)
    {
        db.post_update();
    });
}

TEST_F(GBMDisplayTest, post_update_flip_failure)
{
    using namespace testing;

    auto const crtc_id = get_connected_crtc_id();

    setup_post_update_expectations();

    {
        InSequence s;

        /* New FB flip failure */
        EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(),
                                              crtc_id,
                                              fake.fb_id2,
                                              _, _))
            .Times(Exactly(1))
            .WillOnce(Return(-1));

        /* Release the new (not flipped) BO */
        EXPECT_CALL(mock_gbm, gbm_surface_release_buffer(mock_gbm.fake_gbm.surface, fake.bo2))
            .Times(Exactly(1));

        /* Release the current FB (at destruction time) */
        EXPECT_CALL(mock_gbm, gbm_surface_release_buffer(mock_gbm.fake_gbm.surface, fake.bo1))
            .Times(Exactly(1));
    }

    EXPECT_THROW(
    {
        auto display = create_display(create_platform());

        display->for_each_display_buffer([](mg::DisplayBuffer& db)
        {
            db.post_update();
        });
    }, std::runtime_error);
}

TEST_F(GBMDisplayTest, successful_creation_of_display_reports_successful_setup_of_native_resources)
{
    using namespace ::testing;

    EXPECT_CALL(
        *mock_report,
        report_successful_setup_of_native_resources()).Times(Exactly(1));
    EXPECT_CALL(
        *mock_report,
        report_successful_egl_make_current_on_construction()).Times(Exactly(1));

    EXPECT_CALL(
        *mock_report,
        report_successful_egl_buffer_swap_on_construction()).Times(Exactly(1));

    EXPECT_CALL(
        *mock_report,
        report_successful_drm_mode_set_crtc_on_construction()).Times(Exactly(1));

    EXPECT_CALL(
        *mock_report,
        report_successful_display_construction()).Times(Exactly(1));

    EXPECT_CALL(
        *mock_report,
        report_egl_configuration(mock_egl.fake_egl_display,mock_egl.fake_configs[0])).Times(Exactly(1));

    auto display = std::make_shared<mgg::GBMDisplay>(
                        create_platform(),
                        std::make_shared<mtd::NullVideoDevices>(),
                        std::make_shared<mg::DefaultDisplayConfigurationPolicy>(),
                        mock_report);
}

TEST_F(GBMDisplayTest, outputs_correct_string_for_successful_setup_of_native_resources)
{
    using namespace ::testing;

    auto logger = std::make_shared<MockLogger>();
    auto reporter = std::make_shared<ml::DisplayReport>(logger);

    EXPECT_CALL(
        *logger,
        log(Eq(ml::Logger::informational),
            StrEq("Successfully setup native resources."),
            StrEq("graphics"))).Times(Exactly(1));

    reporter->report_successful_setup_of_native_resources();
}

TEST_F(GBMDisplayTest, outputs_correct_string_for_successful_egl_make_current_on_construction)
{
    using namespace ::testing;

    auto logger = std::make_shared<MockLogger>();
    auto reporter = std::make_shared<ml::DisplayReport>(logger);

    EXPECT_CALL(
        *logger,
        log(Eq(ml::Logger::informational),
            StrEq("Successfully made egl context current on construction."),
            StrEq("graphics"))).Times(Exactly(1));

    reporter->report_successful_egl_make_current_on_construction();
}

TEST_F(GBMDisplayTest, outputs_correct_string_for_successful_egl_buffer_swap_on_construction)
{
    using namespace ::testing;

    auto logger = std::make_shared<MockLogger>();
    auto reporter = std::make_shared<ml::DisplayReport>(logger);

    EXPECT_CALL(
        *logger,
        log(Eq(ml::Logger::informational),
            StrEq("Successfully performed egl buffer swap on construction."),
            StrEq("graphics"))).Times(Exactly(1));

    reporter->report_successful_egl_buffer_swap_on_construction();
}

TEST_F(GBMDisplayTest, outputs_correct_string_for_successful_drm_mode_set_crtc_on_construction)
{
    using namespace ::testing;

    auto logger = std::make_shared<MockLogger>();
    auto reporter = std::make_shared<ml::DisplayReport>(logger);

    EXPECT_CALL(
        *logger,
        log(Eq(ml::Logger::informational),
            StrEq("Successfully performed drm mode setup on construction."),
            StrEq("graphics"))).Times(Exactly(1));

    reporter->report_successful_drm_mode_set_crtc_on_construction();
}

TEST_F(GBMDisplayTest, constructor_throws_if_egl_mesa_drm_image_not_supported)
{
    using namespace ::testing;

    const char* egl_exts = "EGL_KHR_image EGL_KHR_image_base";

    EXPECT_CALL(mock_egl, eglQueryString(_,EGL_EXTENSIONS))
    .WillOnce(Return(egl_exts));

    EXPECT_THROW(
    {
        auto display = create_display(create_platform());
    }, std::runtime_error);
}

TEST_F(GBMDisplayTest, constructor_throws_if_gl_oes_image_not_supported)
{
    using namespace ::testing;

    const char* gl_exts = "GL_OES_texture_npot GL_OES_blend_func_separate";

    EXPECT_CALL(mock_gl, glGetString(GL_EXTENSIONS))
    .WillOnce(Return(reinterpret_cast<const GLubyte*>(gl_exts)));

    EXPECT_THROW(
    {
        auto display = create_display(create_platform());
    }, std::runtime_error);
}

TEST_F(GBMDisplayTest, for_each_display_buffer_calls_callback)
{
    using namespace ::testing;

    auto display = create_display(create_platform());

    int callback_count{0};

    display->for_each_display_buffer([&](mg::DisplayBuffer&)
    {
        callback_count++;
    });

    EXPECT_NE(0, callback_count);
}

TEST_F(GBMDisplayTest, constructor_sets_vt_graphics_mode)
{
    using namespace testing;

    auto mock_vt = std::make_shared<MockVirtualTerminal>();

    EXPECT_CALL(*mock_vt, set_graphics_mode())
        .Times(1);

    auto platform = std::make_shared<mgg::GBMPlatform>(null_report, mock_vt);

    auto display = create_display(platform);
}

TEST_F(GBMDisplayTest, pause_drops_drm_master)
{
    using namespace testing;

    EXPECT_CALL(mock_drm, drmDropMaster(mock_drm.fake_drm.fd()))
        .Times(1);

    auto display = create_display(create_platform());

    display->pause();
}

TEST_F(GBMDisplayTest, resume_sets_drm_master)
{
    using namespace testing;

    EXPECT_CALL(mock_drm, drmSetMaster(mock_drm.fake_drm.fd()))
        .Times(1);

    auto display = create_display(create_platform());

    display->resume();
}

TEST_F(GBMDisplayTest, set_or_drop_drm_master_failure_throws_and_reports_error)
{
    using namespace testing;

    EXPECT_CALL(mock_drm, drmDropMaster(_))
        .WillOnce(SetErrnoAndReturn(EACCES, -EACCES));

    EXPECT_CALL(mock_drm, drmSetMaster(_))
        .WillOnce(SetErrnoAndReturn(EPERM, -EPERM));

    EXPECT_CALL(*mock_report, report_drm_master_failure(EACCES));
    EXPECT_CALL(*mock_report, report_drm_master_failure(EPERM));

    auto platform = std::make_shared<mgg::GBMPlatform>(
                        mock_report,
                        std::make_shared<mtd::NullVirtualTerminal>());
    auto display = std::make_shared<mgg::GBMDisplay>(
                        platform,
                        std::make_shared<mtd::NullVideoDevices>(),
                        std::make_shared<mg::DefaultDisplayConfigurationPolicy>(),
                        mock_report);

    EXPECT_THROW({
        display->pause();
    }, std::runtime_error);

    EXPECT_THROW({
        display->resume();
    }, std::runtime_error);
}

TEST_F(GBMDisplayTest, configuration_change_registers_video_devices_handler)
{
    using namespace testing;

    auto mock_video_devices = std::make_shared<MockVideoDevices>();

    auto display = std::make_shared<mgg::GBMDisplay>(
                        create_platform(),
                        mock_video_devices,
                        std::make_shared<mg::DefaultDisplayConfigurationPolicy>(),
                        null_report);

    mir::AsioMainLoop ml;

    EXPECT_CALL(*mock_video_devices, register_change_handler(Ref(ml),_));

    display->register_configuration_change_handler(ml, []{});
}
