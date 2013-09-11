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

#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/display_configuration.h"
#include "src/server/graphics/gbm/gbm_platform.h"

#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_gl.h"
#include "mir/graphics/null_display_report.h"
#include "mir/graphics/default_display_configuration_policy.h"
#include "mir_test_doubles/null_virtual_terminal.h"

#include "mir_test_framework/udev_environment.h"

#include "mir_test_doubles/mock_drm.h"
#include "mir_test_doubles/mock_gbm.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <unordered_set>

namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace mtf = mir::mir_test_framework;

namespace
{

class ClonedDisplayConfigurationPolicy : public mg::DisplayConfigurationPolicy
{
public:
    void apply_to(mg::DisplayConfiguration& conf)
    {
        conf.for_each_output(
            [&](mg::DisplayConfigurationOutput const& conf_output)
            {
                if (conf_output.connected && conf_output.modes.size() > 0)
                {
                    conf.configure_output(conf_output.id, true, geom::Point{0, 0},
                                          conf_output.preferred_mode_index, mir_power_mode_on);
                }
                else
                {
                    conf.configure_output(conf_output.id, false, conf_output.top_left,
                                          conf_output.current_mode_index, mir_power_mode_on);
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
            [&](mg::DisplayConfigurationOutput const& conf_output)
            {
                if (conf_output.connected && conf_output.modes.size() > 0)
                {
                    conf.configure_output(conf_output.id, true, geom::Point{max_x, 0},
                                          conf_output.preferred_mode_index, mir_power_mode_on);
                    max_x += conf_output.modes[conf_output.preferred_mode_index].size.width.as_int();
                }
                else
                {
                    conf.configure_output(conf_output.id, false, conf_output.top_left,
                                          conf_output.current_mode_index, mir_power_mode_on);
                }
            });
    }
};

class GBMDisplayMultiMonitorTest : public ::testing::Test
{
public:
    GBMDisplayMultiMonitorTest()
    {
        using namespace testing;

        /* Needed for display start-up */
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
            std::make_shared<mg::NullDisplayReport>(),
            std::make_shared<mtd::NullVirtualTerminal>());
    }

    std::shared_ptr<mg::Display> create_display_cloned(
        std::shared_ptr<mg::Platform> const& platform)
    {
        auto conf_policy = std::make_shared<ClonedDisplayConfigurationPolicy>();
        return platform->create_display(conf_policy);
    }

    std::shared_ptr<mg::Display> create_display_side_by_side(
        std::shared_ptr<mg::Platform> const& platform)
    {
        auto conf_policy = std::make_shared<SideBySideDisplayConfigurationPolicy>();
        return platform->create_display(conf_policy);
    }

    void setup_outputs(int n)
    {
        using fake = mtd::FakeDRMResources;

        mtd::FakeDRMResources& resources(mock_drm.fake_drm);

        modes0.clear();
        modes0.push_back(fake::create_mode(1920, 1080, 138500, 2080, 1111, fake::NormalMode));
        modes0.push_back(fake::create_mode(1920, 1080, 148500, 2200, 1125, fake::PreferredMode));
        modes0.push_back(fake::create_mode(1680, 1050, 119000, 1840, 1080, fake::NormalMode));
        modes0.push_back(fake::create_mode(832, 624, 57284, 1152, 667, fake::NormalMode));

        geom::Size const connector_physical_size_mm{1597, 987};

        resources.reset();

        uint32_t const crtc_base_id{10};
        uint32_t const encoder_base_id{20};
        uint32_t const connector_base_id{30};

        for (int i = 0; i < n; i++)
        {
            uint32_t const crtc_id{crtc_base_id + i};
            uint32_t const encoder_id{encoder_base_id + i};
            uint32_t const all_crtcs_mask{0xff};

            crtc_ids.push_back(crtc_id);
            resources.add_crtc(crtc_id, drmModeModeInfo());

            encoder_ids.push_back(encoder_id);
            resources.add_encoder(encoder_id, crtc_id, all_crtcs_mask);
        }

        for (int i = 0; i < n; i++)
        {
            uint32_t const connector_id{connector_base_id + i};

            connector_ids.push_back(connector_id);
            resources.add_connector(connector_id, DRM_MODE_CONNECTOR_VGA,
                                    DRM_MODE_CONNECTED, encoder_ids[i],
                                    modes0, encoder_ids, connector_physical_size_mm);
        }

        resources.prepare();
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
};

}

TEST_F(GBMDisplayMultiMonitorTest, create_display_sets_all_connected_crtcs)
{
    using namespace testing;

    int const num_outputs{3};
    uint32_t const fb_id{66};

    setup_outputs(num_outputs);

    /* Create DRM FBs */
    EXPECT_CALL(mock_drm, drmModeAddFB(mock_drm.fake_drm.fd(),
                                       _, _, _, _, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<7>(fb_id), Return(0)));

    ExpectationSet crtc_setups;

    /* All crtcs are set */
    for (int i = 0; i < num_outputs; i++)
    {
        crtc_setups += EXPECT_CALL(mock_drm,
                                   drmModeSetCrtc(mock_drm.fake_drm.fd(),
                                                  crtc_ids[i], fb_id,
                                                  _, _,
                                                  Pointee(connector_ids[i]),
                                                  _, _))
                           .Times(AtLeast(1));
    }

    /* All crtcs are restored at teardown */
    for (int i = 0; i < num_outputs; i++)
    {
        EXPECT_CALL(mock_drm, drmModeSetCrtc(mock_drm.fake_drm.fd(),
                                             crtc_ids[i], Ne(fb_id),
                                             _, _,
                                             Pointee(connector_ids[i]),
                                             _, _))
            .Times(1)
            .After(crtc_setups);
    }

    auto display = create_display_cloned(create_platform());
}

TEST_F(GBMDisplayMultiMonitorTest, create_display_creates_shared_egl_contexts)
{
    using namespace testing;

    int const num_outputs{3};
    EGLContext const shared_context{reinterpret_cast<EGLContext>(0x77)};

    setup_outputs(num_outputs);

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

TEST_F(GBMDisplayMultiMonitorTest, post_update_flips_all_connected_crtcs)
{
    using namespace testing;

    int const num_outputs{3};
    uint32_t const fb_id{66};
    std::vector<void*> user_data(num_outputs, nullptr);

    setup_outputs(num_outputs);

    /* Create DRM FBs */
    EXPECT_CALL(mock_drm, drmModeAddFB(mock_drm.fake_drm.fd(),
                                       _, _, _, _, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<7>(fb_id), Return(0)));

    /* All crtcs are flipped */
    for (int i = 0; i < num_outputs; i++)
    {
        EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(),
                                              crtc_ids[i], fb_id,
                                              _, _))
            .Times(1)
            .WillOnce(DoAll(SaveArg<4>(&user_data[i]), Return(0)));

        /* Emit fake DRM page-flip events */
        EXPECT_EQ(1, write(mock_drm.fake_drm.write_fd(), "a", 1));
    }

    /* Handle the events properly */
    EXPECT_CALL(mock_drm, drmHandleEvent(mock_drm.fake_drm.fd(), _))
        .Times(num_outputs)
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[0]), Return(0)))
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[1]), Return(0)))
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[2]), Return(0)));

    auto display = create_display_cloned(create_platform());

    display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
    {
        buffer.post_update();
    });
}

namespace
{

struct FBIDContainer
{
    FBIDContainer(uint32_t base_fb_id) : last_fb_id{base_fb_id} {}

    int add_fb(int, uint32_t, uint32_t, uint8_t,
               uint8_t, uint32_t, uint32_t,
               uint32_t *buf_id)
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

TEST_F(GBMDisplayMultiMonitorTest, create_display_uses_different_drm_fbs_for_side_by_side)
{
    using namespace testing;

    int const num_outputs{3};
    uint32_t const base_fb_id{66};
    FBIDContainer fb_id_container{base_fb_id};

    setup_outputs(num_outputs);

    /* Create DRM FBs */
    EXPECT_CALL(mock_drm, drmModeAddFB(mock_drm.fake_drm.fd(),
                                       _, _, _, _, _, _, _))
        .Times(num_outputs)
        .WillRepeatedly(Invoke(&fb_id_container, &FBIDContainer::add_fb));

    ExpectationSet crtc_setups;

    /* All crtcs are set */
    for (int i = 0; i < num_outputs; i++)
    {
        crtc_setups += EXPECT_CALL(mock_drm,
                                   drmModeSetCrtc(mock_drm.fake_drm.fd(),
                                                  crtc_ids[i],
                                                  IsValidFB(&fb_id_container),
                                                  _, _,
                                                  Pointee(connector_ids[i]),
                                                  _, _))
                           .Times(AtLeast(1));
    }

    /* All crtcs are restored at teardown */
    for (int i = 0; i < num_outputs; i++)
    {
        EXPECT_CALL(mock_drm, drmModeSetCrtc(mock_drm.fake_drm.fd(),
                                             crtc_ids[i], 0,
                                             _, _,
                                             Pointee(connector_ids[i]),
                                             _, _))
            .Times(1)
            .After(crtc_setups);
    }

    auto display = create_display_side_by_side(create_platform());
}
