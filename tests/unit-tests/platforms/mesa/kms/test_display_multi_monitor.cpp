/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/platform.h"

#include "src/platforms/mesa/server/kms/platform.h"

#include "mir/test/doubles/null_emergency_cleanup.h"
#include "src/server/report/null_report_factory.h"
#include "mir/test/doubles/null_virtual_terminal.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/test/doubles/stub_gl_config.h"
#include "mir/test/doubles/stub_gl_program_factory.h"

#include "mir_test_framework/udev_environment.h"

#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_gbm.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <unordered_set>
#include <fcntl.h>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{

class ClonedDisplayConfigurationPolicy : public mg::DisplayConfigurationPolicy
{
public:
    void apply_to(mg::DisplayConfiguration& conf)
    {
        conf.for_each_output(
            [&](mg::UserDisplayConfigurationOutput& conf_output)
            {
                if (conf_output.connected && conf_output.modes.size() > 0)
                {
                    conf_output.used = true;
                    conf_output.top_left = geom::Point{0, 0};
                    conf_output.current_mode_index =
                        conf_output.preferred_mode_index;
                    conf_output.power_mode = mir_power_mode_on;
                }
                else
                {
                    conf_output.used = false;
                    conf_output.power_mode = mir_power_mode_off;
                }
            });
    }
};

class SideBySideDisplayConfigurationPolicy : public mg::DisplayConfigurationPolicy
{
public:
    void apply_to(mg::DisplayConfiguration& conf)
    {
        int max_x = 0;

        conf.for_each_output(
            [&](mg::UserDisplayConfigurationOutput& conf_output)
            {
                if (conf_output.connected && conf_output.modes.size() > 0)
                {
                    conf_output.used = true;
                    conf_output.top_left = geom::Point{max_x, 0};
                    conf_output.current_mode_index =
                        conf_output.preferred_mode_index;
                    conf_output.power_mode = mir_power_mode_on;
                    conf_output.orientation = mir_orientation_normal;
                    max_x += conf_output.modes[conf_output.preferred_mode_index].size.width.as_int();
                }
                else
                {
                    conf_output.used = false;
                    conf_output.power_mode = mir_power_mode_off;
                }
            });
    }
};

class MesaDisplayMultiMonitorTest : public ::testing::Test
{
public:
    MesaDisplayMultiMonitorTest()
        : drm_fd{open(drm_device, 0, 0)}
    {
        using namespace testing;

        /* Needed for display start-up */
        ON_CALL(mock_egl, eglChooseConfig(_,_,_,1,_))
            .WillByDefault(DoAll(SetArgPointee<2>(mock_egl.fake_configs[0]),
                                 SetArgPointee<4>(1),
                                 Return(EGL_TRUE)));

        mock_egl.provide_egl_extensions();
        mock_gl.provide_gles_extensions();

        /*
         * Silence uninteresting calls called when cleaning up resources in
         * the MockGBM destructor, and which are not handled by NiceMock<>.
         */
        EXPECT_CALL(mock_gbm, gbm_bo_get_device(_))
            .Times(AtLeast(0));
        EXPECT_CALL(mock_gbm, gbm_device_get_fd(_))
            .Times(AtLeast(0))
            .WillRepeatedly(Return(drm_fd));

        fake_devices.add_standard_device("standard-drm-devices");

        // Remove all outputs from all but one of the DRM devices we access;
        // these tests are not set up to test hybrid.
        mock_drm.reset("/dev/dri/card1");
        mock_drm.reset("/dev/dri/card2");
    }

    std::shared_ptr<mgm::Platform> create_platform()
    {
        return std::make_shared<mgm::Platform>(
               mir::report::null_display_report(),
               std::make_shared<mtd::NullVirtualTerminal>(),
               *std::make_shared<mtd::NullEmergencyCleanup>(),
               mgm::BypassOption::allowed);
    }

    std::shared_ptr<mg::Display> create_display_cloned(
        std::shared_ptr<mgm::Platform> const& platform)
    {
        return platform->create_display(
            std::make_shared<ClonedDisplayConfigurationPolicy>(),
            std::make_shared<mtd::StubGLConfig>());
    }

    std::shared_ptr<mg::Display> create_display_side_by_side(
        std::shared_ptr<mg::Platform> const& platform)
    {
        return platform->create_display(
            std::make_shared<SideBySideDisplayConfigurationPolicy>(),
            std::make_shared<mtd::StubGLConfig>());
    }

    void setup_outputs(int connected, int disconnected)
    {
        using fake = mtd::FakeDRMResources;

        modes0.clear();
        modes0.push_back(fake::create_mode(1920, 1080, 138500, 2080, 1111, fake::NormalMode));
        modes0.push_back(fake::create_mode(1920, 1080, 148500, 2200, 1125, fake::PreferredMode));
        modes0.push_back(fake::create_mode(1680, 1050, 119000, 1840, 1080, fake::NormalMode));
        modes0.push_back(fake::create_mode(832, 624, 57284, 1152, 667, fake::NormalMode));

        geom::Size const connector_physical_size_mm{1597, 987};

        mock_drm.reset(drm_device);

        uint32_t const crtc_base_id{10};
        uint32_t const encoder_base_id{20};
        uint32_t const connector_base_id{30};

        for (int i = 0; i < connected; i++)
        {
            uint32_t const crtc_id{crtc_base_id + i};
            uint32_t const encoder_id{encoder_base_id + i};
            uint32_t const all_crtcs_mask{0xff};

            crtc_ids.push_back(crtc_id);
            mock_drm.add_crtc(drm_device, crtc_id, drmModeModeInfo());

            encoder_ids.push_back(encoder_id);
            mock_drm.add_encoder(drm_device, encoder_id, crtc_id, all_crtcs_mask);
        }

        for (int i = 0; i < connected; i++)
        {
            uint32_t const connector_id{connector_base_id + i};

            connector_ids.push_back(connector_id);
            mock_drm.add_connector(
                drm_device,
                connector_id,
                DRM_MODE_CONNECTOR_VGA,
                DRM_MODE_CONNECTED,
                encoder_ids[i],
                modes0,
                encoder_ids,
                connector_physical_size_mm);
        }

        for (int i = 0; i < disconnected; i++)
        {
            uint32_t const connector_id{connector_base_id + connected + i};

            connector_ids.push_back(connector_id);
            mock_drm.add_connector(
                drm_device,
                connector_id,
                DRM_MODE_CONNECTOR_VGA,
                DRM_MODE_DISCONNECTED,
                0,
                modes_empty,
                encoder_ids,
                geom::Size{});
        }

        mock_drm.prepare(drm_device);
    }


    testing::NiceMock<mtd::MockEGL> mock_egl;
    testing::NiceMock<mtd::MockGL> mock_gl;
    testing::NiceMock<mtd::MockDRM> mock_drm;
    testing::NiceMock<mtd::MockGBM> mock_gbm;

    std::vector<drmModeModeInfo> modes0;
    std::vector<drmModeModeInfo> modes_empty;
    std::vector<uint32_t> crtc_ids;
    std::vector<uint32_t> encoder_ids;
    std::vector<uint32_t> connector_ids;

    mtf::UdevEnvironment fake_devices;

    char const* const drm_device = "/dev/dri/card0";
    int const drm_fd;
};

}

TEST_F(MesaDisplayMultiMonitorTest, create_display_sets_all_connected_crtcs)
{
    using namespace testing;

    int const num_connected_outputs{3};
    int const num_disconnected_outputs{2};
    uint32_t const fb_id{66};

    setup_outputs(num_connected_outputs, num_disconnected_outputs);

    /* Create DRM FBs */
    EXPECT_CALL(mock_drm, drmModeAddFB2(mtd::IsFdOfDevice(drm_device),
                                        _, _, _, _, _, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<7>(fb_id), Return(0)));

    ExpectationSet crtc_setups;

    /* All crtcs are set */
    for (int i = 0; i < num_connected_outputs; i++)
    {
        crtc_setups += EXPECT_CALL(mock_drm,
                                   drmModeSetCrtc(mtd::IsFdOfDevice(drm_device),
                                                  crtc_ids[i], fb_id,
                                                  _, _,
                                                  Pointee(connector_ids[i]),
                                                  _, _))
                           .Times(AtLeast(1));
    }

    /* All crtcs are restored at teardown */
    for (int i = 0; i < num_connected_outputs; i++)
    {
        EXPECT_CALL(mock_drm, drmModeSetCrtc(mtd::IsFdOfDevice(drm_device),
                                             crtc_ids[i], Ne(fb_id),
                                             _, _,
                                             Pointee(connector_ids[i]),
                                             _, _))
            .Times(1)
            .After(crtc_setups);
    }

    auto display = create_display_cloned(create_platform());
}

TEST_F(MesaDisplayMultiMonitorTest, create_display_creates_shared_egl_contexts)
{
    using namespace testing;

    int const num_connected_outputs{3};
    int const num_disconnected_outputs{2};
    EGLContext const shared_context{reinterpret_cast<EGLContext>(0x77)};

    setup_outputs(num_connected_outputs, num_disconnected_outputs);

    /* Will create only one shared context */
    EXPECT_CALL(mock_egl, eglCreateContext(_, _, EGL_NO_CONTEXT, _))
        .WillOnce(Return(shared_context));

    /* Will use the shared context when creating other contexts */
    EXPECT_CALL(mock_egl, eglCreateContext(_, _, shared_context, _))
        .Times(AtLeast(1));

    {
        InSequence s;

        /* Contexts are made current to initialize DisplayBuffers */
        EXPECT_CALL(mock_egl, eglMakeCurrent(_,_,_,Ne(shared_context)))
            .Times(AtLeast(1));

        /* The shared context is made current finally */
        EXPECT_CALL(mock_egl, eglMakeCurrent(_,_,_,shared_context))
            .Times(1);

        /* Contexts are released at teardown */
        EXPECT_CALL(mock_egl, eglMakeCurrent(_,_,_,EGL_NO_CONTEXT))
            .Times(AtLeast(1));
    }

    auto display = create_display_cloned(create_platform());
}

namespace
{

ACTION_P(InvokePageFlipHandler, param)
{
    int const dont_care{0};
    char dummy;

    arg1->page_flip_handler(dont_care, dont_care, dont_care, dont_care, *param);
    ASSERT_EQ(1, read(arg0, &dummy, 1));
}

}

TEST_F(MesaDisplayMultiMonitorTest, flip_flips_all_connected_crtcs)
{
    using namespace testing;

    int const num_connected_outputs{3};
    int const num_disconnected_outputs{2};
    uint32_t const fb_id{66};
    std::vector<void*> user_data(num_connected_outputs, nullptr);

    setup_outputs(num_connected_outputs, num_disconnected_outputs);

    /* Create DRM FBs */
    EXPECT_CALL(mock_drm, drmModeAddFB2(mtd::IsFdOfDevice(drm_device),
                                        _, _, _, _, _, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<7>(fb_id), Return(0)));

    /* All crtcs are flipped */
    for (int i = 0; i < num_connected_outputs; i++)
    {
        EXPECT_CALL(mock_drm, drmModePageFlip(mtd::IsFdOfDevice(drm_device),
                                              crtc_ids[i], fb_id,
                                              _, _))
            .Times(2)
            .WillRepeatedly(DoAll(SaveArg<4>(&user_data[i]), Return(0)));

        /* Emit fake DRM page-flip events */
        mock_drm.generate_event_on(drm_device);
    }

    /* Handle the events properly */
    EXPECT_CALL(mock_drm, drmHandleEvent(mtd::IsFdOfDevice(drm_device), _))
        .Times(num_connected_outputs)
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[0]), Return(0)))
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[1]), Return(0)))
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[2]), Return(0)));

    auto display = create_display_cloned(create_platform());

    /* First frame: Page flips are scheduled, but not waited for */
    display->for_each_display_sync_group([](mg::DisplaySyncGroup& group)
    {
        group.post();
    });

    /* Second frame: Previous page flips finish (drmHandleEvent) and new ones
       are scheduled */
    display->for_each_display_sync_group([](mg::DisplaySyncGroup& group)
    {
        group.post();
    });
}

namespace
{

struct FBIDContainer
{
    FBIDContainer(uint32_t base_fb_id) : last_fb_id{base_fb_id} {}

    int add_fb2(int, uint32_t, uint32_t, uint32_t,
               uint32_t const[4], uint32_t const[4], uint32_t const[4],
               uint32_t *buf_id, uint32_t)
    {
        *buf_id = last_fb_id;
        fb_ids.insert(last_fb_id);
        ++last_fb_id;
        return 0;
    }

    bool check_fb_id(uint32_t i)
    {
        if (fb_ids.find(i) != fb_ids.end())
        {
            fb_ids.erase(i);
            return true;
        }

        return false;
    }

    std::unordered_set<uint32_t> fb_ids;
    uint32_t last_fb_id;
};

MATCHER_P(IsValidFB, fb_id_container, "") { return fb_id_container->check_fb_id(arg); }

}

TEST_F(MesaDisplayMultiMonitorTest, create_display_uses_different_drm_fbs_for_side_by_side)
{
    using namespace testing;

    int const num_connected_outputs{3};
    int const num_disconnected_outputs{2};
    uint32_t const base_fb_id{66};
    FBIDContainer fb_id_container{base_fb_id};

    setup_outputs(num_connected_outputs, num_disconnected_outputs);

    /* Create DRM FBs */
    EXPECT_CALL(mock_drm, drmModeAddFB2(mtd::IsFdOfDevice(drm_device),
                                        _, _, _, _, _, _, _, _))
        .Times(num_connected_outputs)
        .WillRepeatedly(Invoke(&fb_id_container, &FBIDContainer::add_fb2));

    ExpectationSet crtc_setups;

    /* All crtcs are set */
    for (int i = 0; i < num_connected_outputs; i++)
    {
        crtc_setups += EXPECT_CALL(mock_drm,
                                   drmModeSetCrtc(mtd::IsFdOfDevice(drm_device),
                                                  crtc_ids[i],
                                                  IsValidFB(&fb_id_container),
                                                  _, _,
                                                  Pointee(connector_ids[i]),
                                                  _, _))
                           .Times(AtLeast(1));
    }

    /* All crtcs are restored at teardown */
    for (int i = 0; i < num_connected_outputs; i++)
    {
        EXPECT_CALL(mock_drm, drmModeSetCrtc(mtd::IsFdOfDevice(drm_device),
                                             crtc_ids[i], 0,
                                             _, _,
                                             Pointee(connector_ids[i]),
                                             _, _))
            .Times(1)
            .After(crtc_setups);
    }

    auto display = create_display_side_by_side(create_platform());
}

TEST_F(MesaDisplayMultiMonitorTest, configure_clears_unused_connected_outputs)
{
    using namespace testing;

    int const num_connected_outputs{3};
    int const num_disconnected_outputs{2};

    setup_outputs(num_connected_outputs, num_disconnected_outputs);

    auto display = create_display_cloned(create_platform());

    Mock::VerifyAndClearExpectations(&mock_drm);

    /* All unused connected outputs are cleared */
    for (int i = 0; i < num_connected_outputs; i++)
    {
        EXPECT_CALL(mock_drm,
                    drmModeSetCursor(mtd::IsFdOfDevice(drm_device),
                                     crtc_ids[i], 0, 0, 0))
                        .Times(1);
        EXPECT_CALL(mock_drm,
                    drmModeSetCrtc(mtd::IsFdOfDevice(drm_device),
                                   crtc_ids[i], 0, 0, 0,
                                   nullptr, 0, nullptr))
                        .Times(1);
    }

    /* Set all outputs to unused */
    auto conf = display->configuration();

    conf->for_each_output(
        [&](mg::UserDisplayConfigurationOutput& output)
        {
            output.used = false;
            output.current_mode_index = output.preferred_mode_index;
            output.current_format = mir_pixel_format_xrgb_8888;
            output.power_mode = mir_power_mode_on;
        });

    display->configure(*conf);

    Mock::VerifyAndClearExpectations(&mock_drm);

    /* All crtcs are restored at teardown */
    for (int i = 0; i < num_connected_outputs; i++)
    {
        EXPECT_CALL(mock_drm,
                    drmModeSetCrtc(mtd::IsFdOfDevice(drm_device), crtc_ids[i],
                                   0, _, _, Pointee(connector_ids[i]),
                                   _, _))
                        .Times(1);
    }
}

TEST_F(MesaDisplayMultiMonitorTest, resume_clears_unused_connected_outputs)
{
    using namespace testing;

    int const num_connected_outputs{3};
    int const num_disconnected_outputs{2};

    setup_outputs(num_connected_outputs, num_disconnected_outputs);

    auto display = create_display_cloned(create_platform());

    /* Set all outputs to unused */
    auto conf = display->configuration();

    conf->for_each_output(
        [&](mg::UserDisplayConfigurationOutput& output)
        {
            output.used = false;
            output.current_mode_index = output.preferred_mode_index;
            output.current_format = mir_pixel_format_xrgb_8888;
            output.power_mode = mir_power_mode_on;
        });

    display->configure(*conf);

    display->pause();

    Mock::VerifyAndClearExpectations(&mock_drm);

    /* All unused connected outputs are cleared */
    for (int i = 0; i < num_connected_outputs; i++)
    {
        EXPECT_CALL(mock_drm,
                    drmModeSetCrtc(mtd::IsFdOfDevice(drm_device),
                                   crtc_ids[i], 0, 0, 0,
                                   nullptr, 0, nullptr))
                        .Times(1);
    }

    display->resume();

    Mock::VerifyAndClearExpectations(&mock_drm);

    /* All crtcs are restored at teardown */
    for (int i = 0; i < num_connected_outputs; i++)
    {
        EXPECT_CALL(mock_drm,
                    drmModeSetCrtc(mtd::IsFdOfDevice(drm_device), crtc_ids[i],
                                   0, _, _, Pointee(connector_ids[i]),
                                   _, _))
                        .Times(1);
    }
}
