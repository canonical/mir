/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "kms_display_configuration.h"
#include "mir/exception.h"

namespace mgg = mir::graphics::gbm;
namespace geom = mir::geometry;

namespace
{

struct CrtcDeleter
{
    void operator()(drmModeCrtc* p) { if (p) drmModeFreeCrtc(p); }
};

struct EncoderDeleter
{
    void operator()(drmModeEncoder* p) { if (p) drmModeFreeEncoder(p); }
};

struct ResourcesDeleter
{
    void operator()(drmModeRes* p) { if (p) drmModeFreeResources(p); }
};

struct ConnectorDeleter
{
    void operator()(drmModeConnector* p) { if (p) drmModeFreeConnector(p); }
};

typedef std::unique_ptr<drmModeCrtc,CrtcDeleter> CrtcUPtr;
typedef std::unique_ptr<drmModeEncoder,EncoderDeleter> EncoderUPtr;
typedef std::unique_ptr<drmModeRes,ResourcesDeleter> ResourcesUPtr;
typedef std::unique_ptr<drmModeConnector,ConnectorDeleter> ConnectorUPtr;

bool kms_modes_are_equal(drmModeModeInfo const& info1, drmModeModeInfo const& info2)
{
    return (info1.clock == info2.clock &&
            info1.hdisplay == info2.hdisplay &&
            info1.hsync_start == info2.hsync_start &&
            info1.hsync_end == info2.hsync_end &&
            info1.htotal == info2.htotal &&
            info1.hskew == info2.hskew &&
            info1.vdisplay == info2.vdisplay &&
            info1.vsync_start == info2.vsync_start &&
            info1.vsync_end == info2.vsync_end &&
            info1.vtotal == info2.vtotal);
}

EncoderUPtr find_encoder(int drm_fd, drmModeRes const& resources, uint32_t encoder_id)
{
    for (int i = 0; i < resources.count_encoders; i++)
    {
        auto encoder_raw = drmModeGetEncoder(drm_fd, resources.encoders[i]);
        EncoderUPtr encoder{encoder_raw, EncoderDeleter()};

        if (!encoder)
            continue;

        if (encoder->encoder_id == encoder_id)
            return std::move(encoder);
    }

    return EncoderUPtr();
}

CrtcUPtr find_crtc(int drm_fd, drmModeRes const& resources, uint32_t crtc_id)
{
    for (int i = 0; i < resources.count_crtcs; i++)
    {
        auto crtc_raw = drmModeGetCrtc(drm_fd, resources.crtcs[i]);
        CrtcUPtr crtc{crtc_raw, CrtcDeleter()};

        if (!crtc)
            continue;

        if (crtc->crtc_id == crtc_id)
            return std::move(crtc);
    }

    return CrtcUPtr();
}

double calculate_vrefresh_hz(drmModeModeInfo const& mode)
{
    if (mode.htotal == 0 || mode.vtotal == 0)
        return 0.0;

    /* mode.clock is in KHz */
    double vrefresh_hz = mode.clock * 1000.0 / (mode.htotal * mode.vtotal);

    /* Round to first decimal */
    return round(vrefresh_hz * 10.0) / 10.0;
}

}

mgg::KMSDisplayConfiguration::KMSDisplayConfiguration(int drm_fd)
    : drm_fd{drm_fd}
{
    auto resources_raw = drmModeGetResources(drm_fd);
    ResourcesUPtr resources{resources_raw, ResourcesDeleter()};

    if (!resources)
        BOOST_THROW_EXCEPTION(std::runtime_error("Couldn't get DRM resources\n"));

    for (int i = 0; i < resources->count_connectors; i++)
    {
        auto connector_raw = drmModeGetConnector(drm_fd, resources->connectors[i]);
        ConnectorUPtr connector{connector_raw, ConnectorDeleter()};

        if (!connector)
            continue;

        add_output(*resources, *connector);
    }
}

void mgg::KMSDisplayConfiguration::for_each_card(std::function<void(DisplayConfigurationCard const&)> f)
{
    DisplayConfigurationCard const card{DisplayConfigurationCardId{0}};
    f(card);
}

void mgg::KMSDisplayConfiguration::for_each_output(std::function<void(DisplayConfigurationOutput const&)> f)
{
    for (auto const& output : outputs)
        f(output);
}

void mgg::KMSDisplayConfiguration::add_output(drmModeRes const& resources,
                                              drmModeConnector const& connector)
{
    DisplayConfigurationOutputId id(outputs.size());
    DisplayConfigurationCardId card_id{0};
    geom::Size physical_size{geom::Width{connector.mmWidth},
                             geom::Height{connector.mmHeight}};
    bool connected{connector.connection == DRM_MODE_CONNECTED};
    size_t current_mode_index{std::numeric_limits<size_t>::max()};
    std::vector<DisplayConfigurationMode> modes;
    drmModeModeInfo current_mode_info = drmModeModeInfo();

    /* Get information about the current mode */
    auto encoder = find_encoder(drm_fd, resources, connector.encoder_id);
    if (encoder)
    {
        auto crtc = find_crtc(drm_fd, resources, encoder->crtc_id);
        if (crtc)
            current_mode_info = crtc->mode;
    }

    /* Add all the available modes and find the current one */
    for (int m = 0; m < connector.count_modes; m++)
    {
        drmModeModeInfo& mode_info = connector.modes[m];

        geom::Size size{geom::Width{mode_info.hdisplay},
                        geom::Height{mode_info.vdisplay}};

        double vrefresh_hz = calculate_vrefresh_hz(mode_info);

        modes.push_back({size, vrefresh_hz});

        if (kms_modes_are_equal(mode_info, current_mode_info))
            current_mode_index = m;
    }

    /* Add the output */
    outputs.push_back({id, card_id, modes, physical_size,
                       connected, current_mode_index});
}
