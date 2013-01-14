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

#include "mir/exception.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/display.h"
#include "src/graphics/gbm/gbm_platform.h"

#include "mir_test/egl_mock.h"
#include "mir_test/gl_mock.h"
#include "mir_test_doubles/null_display_listener.h"

#include "mock_drm.h"
#include "mock_gbm.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;

namespace
{

/* Helper functions */

drmModeModeInfo create_mode(uint16_t hdisplay, uint16_t vdisplay,
                            uint32_t clock, uint16_t htotal, uint16_t vtotal)
{
    drmModeModeInfo mode = drmModeModeInfo();

    mode.hdisplay = hdisplay;
    mode.vdisplay = vdisplay;
    mode.clock = clock;
    mode.htotal = htotal;
    mode.vtotal = vtotal;

    return mode;
}

drmModeCrtc create_crtc(uint32_t id, drmModeModeInfo mode)
{
    drmModeCrtc crtc = drmModeCrtc();

    crtc.crtc_id = id;
    crtc.mode = mode;

    return crtc;
}

drmModeEncoder create_encoder(uint32_t encoder_id, uint32_t crtc_id)
{
    drmModeEncoder encoder = drmModeEncoder();

    encoder.encoder_id = encoder_id;
    encoder.crtc_id = crtc_id;

    return encoder;
}

drmModeConnector create_connector(uint32_t connector_id, drmModeConnection connection,
                                  uint32_t encoder_id, std::vector<drmModeModeInfo>& modes,
                                  geom::Size const& physical_size)
{
    drmModeConnector connector = drmModeConnector();

    connector.connector_id = connector_id;
    connector.connection = connection;
    connector.encoder_id = encoder_id;
    connector.modes = modes.data();
    connector.count_modes = modes.size();
    connector.mmWidth = physical_size.width.as_uint32_t();
    connector.mmHeight = physical_size.height.as_uint32_t();

    return connector;
}

void outputs_eq(mg::DisplayConfigurationOutput const& expected_out,
                mg::DisplayConfigurationOutput const& out,
                size_t index)
{
    EXPECT_EQ(expected_out.id, out.id) << "output " << index;
    EXPECT_EQ(expected_out.card_id, out.card_id) << "output " << index;
    EXPECT_EQ(expected_out.physical_size_mm, out.physical_size_mm) << "output " << index;
    EXPECT_EQ(expected_out.connected, out.connected) << "output " << index;
    EXPECT_EQ(expected_out.current_mode_index, out.current_mode_index) << "output " << index;

    ASSERT_EQ(expected_out.modes.size(), out.modes.size()) << index;

    for (size_t i = 0; i < expected_out.modes.size(); i++)
    {
        EXPECT_EQ(expected_out.modes[i].size, out.modes[i].size) << "output" << index << "mode " << i;
        EXPECT_EQ(expected_out.modes[i].vrefresh_hz, out.modes[i].vrefresh_hz) << "output" << index << "mode " << i;
    }
}

struct FakeDRMResources
{
    FakeDRMResources()
    {
        modes1.push_back(create_mode(1920, 1080, 138500, 2080, 1111));
        modes1.push_back(create_mode(1920, 1080, 148500, 2200, 1125));
        modes1.push_back(create_mode(1680, 1050, 119000, 1840, 1080));
        modes1.push_back(create_mode(832, 624, 57284, 1152, 667));
    }

    void prepare()
    {
        resources.count_crtcs = crtcs.size();
        for (auto const& crtc: crtcs)
            crtc_ids.push_back(crtc.crtc_id);
        resources.crtcs = crtc_ids.data();

        resources.count_encoders = encoders.size();
        for (auto const& encoder: encoders)
            encoder_ids.push_back(encoder.encoder_id);
        resources.encoders = encoder_ids.data();

        resources.count_connectors = connectors.size();
        for (auto const& connector: connectors)
            connector_ids.push_back(connector.connector_id);
        resources.connectors = connector_ids.data();
    }

    drmModeRes resources;
    std::vector<drmModeCrtc> crtcs;
    std::vector<drmModeEncoder> encoders;
    std::vector<drmModeConnector> connectors;

    std::vector<uint32_t> crtc_ids;
    std::vector<uint32_t> encoder_ids;
    std::vector<uint32_t> connector_ids;

    std::vector<drmModeModeInfo> modes1;
    std::vector<drmModeModeInfo> modes_empty;
};

class GBMDisplayConfigurationTest : public ::testing::Test
{
public:
    GBMDisplayConfigurationTest() :
        null_listener{std::make_shared<mtd::NullDisplayListener>()}
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
    }


    void setup_drm_call_defaults()
    {
        using namespace testing;

        resources.prepare();

        ON_CALL(mock_drm, drmModeGetResources(_))
        .WillByDefault(Return(&resources.resources));

        ON_CALL(mock_drm, drmModeGetCrtc(_, _))
        .WillByDefault(WithArgs<1>(Invoke(this, &GBMDisplayConfigurationTest::find_crtc)));

        ON_CALL(mock_drm, drmModeGetEncoder(_, _))
        .WillByDefault(WithArgs<1>(Invoke(this, &GBMDisplayConfigurationTest::find_encoder)));

        ON_CALL(mock_drm, drmModeGetConnector(_, _))
        .WillByDefault(WithArgs<1>(Invoke(this, &GBMDisplayConfigurationTest::find_connector)));
    }

    drmModeCrtc* find_crtc(uint32_t id)
    {
        for (auto& crtc : resources.crtcs)
        {
            if (crtc.crtc_id == id)
                return &crtc;
        }
        return nullptr;
    }

    drmModeEncoder* find_encoder(uint32_t id)
    {
        for (auto& encoder : resources.encoders)
        {
            if (encoder.encoder_id == id)
                return &encoder;
        }
        return nullptr;
    }

    drmModeConnector* find_connector(uint32_t id)
    {
        for (auto& connector : resources.connectors)
        {
            if (connector.connector_id == id)
                return &connector;
        }
        return nullptr;
    }

    ::testing::NiceMock<mir::EglMock> mock_egl;
    ::testing::NiceMock<mir::GLMock> mock_gl;
    ::testing::NiceMock<mgg::MockDRM> mock_drm;
    ::testing::NiceMock<mgg::MockGBM> mock_gbm;
    std::shared_ptr<mg::DisplayListener> const null_listener;

    FakeDRMResources resources;
};

}

TEST_F(GBMDisplayConfigurationTest, configuration_is_read_correctly)
{
    using namespace ::testing;

    /* Expected results */
    std::vector<mg::DisplayConfigurationMode> const expected_conf_modes
    {
        { geom::Size{geom::Width{1920}, geom::Height{1080}}, 59.9 },
        { geom::Size{geom::Width{1920}, geom::Height{1080}}, 60.0 },
        { geom::Size{geom::Width{1680}, geom::Height{1050}}, 59.9 },
        { geom::Size{geom::Width{832}, geom::Height{624}}, 74.6 }
    };

    std::vector<mg::DisplayConfigurationOutput> const expected_outputs =
    {
        {
            mg::DisplayConfigurationOutputId{0},
            mg::DisplayConfigurationCardId{0},
            expected_conf_modes,
            geom::Size{geom::Width{480}, geom::Height{270}},
            true,
            1
        },
        {
            mg::DisplayConfigurationOutputId{1},
            mg::DisplayConfigurationCardId{0},
            std::vector<mg::DisplayConfigurationMode>(),
            geom::Size(),
            false,
            std::numeric_limits<size_t>::max()
        },
        {
            mg::DisplayConfigurationOutputId{2},
            mg::DisplayConfigurationCardId{0},
            std::vector<mg::DisplayConfigurationMode>(),
            geom::Size(),
            false,
            std::numeric_limits<size_t>::max()
        }
    };

    /* Set up DRM resources */
    resources.crtcs.push_back(create_crtc(10, resources.modes1[1]));

    resources.encoders.push_back(create_encoder(20, 10));
    resources.encoders.push_back(create_encoder(21, 0));

    resources.connectors.push_back(create_connector(30, DRM_MODE_CONNECTED, 20,
                                                    resources.modes1,
                                                    expected_outputs[0].physical_size_mm));
    resources.connectors.push_back(create_connector(31, DRM_MODE_DISCONNECTED, 0,
                                                    resources.modes_empty,
                                                    expected_outputs[1].physical_size_mm));
    resources.connectors.push_back(create_connector(32, DRM_MODE_DISCONNECTED, 21,
                                                    resources.modes_empty,
                                                    expected_outputs[2].physical_size_mm));

    setup_drm_call_defaults();

    /* Test body */
    auto platform = std::make_shared<mgg::GBMPlatform>(null_listener);
    auto display = platform->create_display();

    auto conf = display->configuration();

    size_t output_count{0};

    conf->for_each_output([&](mg::DisplayConfigurationOutput& output)
    {
        ASSERT_LT(output_count, expected_outputs.size());
        outputs_eq(expected_outputs[output_count], output, output_count);
        ++output_count;
    });

    ASSERT_EQ(expected_outputs.size(), output_count);
}
