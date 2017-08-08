/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <epoxy/egl.h>

#include "kms_display_configuration.h"
#include "kms-utils/drm_mode_resources.h"
#include "mir/graphics/pixel_format_utils.h"
#include "mir/graphics/egl_error.h"

#include <cmath>
#include <limits>

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <algorithm>

namespace mg = mir::graphics;
namespace mge = mir::graphics::eglstream;
namespace mgk = mir::graphics::kms;

namespace geom = mir::geometry;

namespace
{

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

double calculate_vrefresh_hz(drmModeModeInfo const& mode)
{
    if (mode.htotal == 0 || mode.vtotal == 0)
        return 0.0;

    /* mode.clock is in KHz */
    double hz = (mode.clock * 100000LL /
                 ((long)mode.htotal * (long)mode.vtotal)
                ) / 100.0;

    // Actually we don't need floating point at all for this...
    // TODO: Consider converting our structs to fixed-point ints
    return hz;
}

mg::DisplayConfigurationOutputType
kms_connector_type_to_output_type(uint32_t connector_type)
{
    return static_cast<mg::DisplayConfigurationOutputType>(connector_type);
}

std::vector<std::shared_ptr<mge::kms::EGLOutput>> create_outputs(int drm_fd, EGLDisplay display)
{
    mgk::DRMModeResources resources{drm_fd};

    std::vector<std::shared_ptr<mge::kms::EGLOutput>> outputs;

    for (auto const& connector : resources.connectors())
    {
        EGLOutputPortEXT port;
        int num_ports;
        EGLAttrib const select_connector[] = {
            EGL_DRM_CONNECTOR_EXT, static_cast<EGLAttrib>(connector->connector_id),
            EGL_NONE
        };
        if (eglGetOutputPortsEXT(display, select_connector, &port, 1, &num_ports) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to find EGLOutputPort corresponding to DRM connector"));
        }
        if (num_ports != 1)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("Failed to find EGLOutputPort corresponding to DRM connector"));
        }
        outputs.push_back(std::make_shared<mge::kms::EGLOutput>(drm_fd, display, port));
        auto& output = outputs.back();

        // TODO: This should all be owned by mgek::EGLOutput

        output->id = mg::DisplayConfigurationOutputId{static_cast<int>(connector->connector_id)};
        output->card_id = mg::DisplayConfigurationCardId{0};
        output->type = kms_connector_type_to_output_type(connector->connector_type);
        output->physical_size_mm = geom::Size{connector->mmWidth, connector->mmHeight};
        output->connected = connector->connection == DRM_MODE_CONNECTED;
        output->pixel_formats = {
            mir_pixel_format_argb_8888,
            mir_pixel_format_xrgb_8888
        };

        uint32_t const invalid_mode_index = std::numeric_limits<uint32_t>::max();
        output->current_mode_index = invalid_mode_index;
        output->preferred_mode_index = invalid_mode_index;

        /* Get information about the current mode */
        mgk::DRMModeCrtcUPtr current_crtc;
        if (connector->encoder_id)
        {
            auto encoder = mgk::get_encoder(drm_fd, connector->encoder_id);
            if (encoder->crtc_id)
            {
                current_crtc = mgk::get_crtc(drm_fd, encoder->crtc_id);
            }
        }

         /* Add all the available modes and find the current and preferred one */
         for (int m = 0; m < connector->count_modes; m++)
         {
             drmModeModeInfo &mode_info = connector->modes[m];

             geom::Size size{mode_info.hdisplay, mode_info.vdisplay};

             double vrefresh_hz = calculate_vrefresh_hz(mode_info);

             output->modes.push_back({size, vrefresh_hz});

             if (current_crtc && kms_modes_are_equal(mode_info, current_crtc->mode))
                 output->current_mode_index = m;

             if ((mode_info.type & DRM_MODE_TYPE_PREFERRED) == DRM_MODE_TYPE_PREFERRED)
                 output->preferred_mode_index = m;
         }


        output->current_format = mir_pixel_format_xrgb_8888;
        output->power_mode = mir_power_mode_on;
        output->orientation = mir_orientation_normal;
        output->scale = 1.0f;
        output->form_factor = mir_form_factor_monitor;
        output->used = false;
    }

    return outputs;
}

mg::DisplayConfigurationCard create_card(int drm_fd)
{
    mgk::DRMModeResources resources{drm_fd};

    size_t max_outputs = std::min(resources.num_crtcs(), resources.num_connectors());
    return {mg::DisplayConfigurationCardId{0}, max_outputs};
}

}

mge::KMSDisplayConfiguration::KMSDisplayConfiguration(int drm_fd, EGLDisplay dpy)
    : drm_fd{drm_fd},
      card{create_card(drm_fd)},
      outputs{create_outputs(drm_fd, dpy)}
{
    update();
}

mge::KMSDisplayConfiguration::KMSDisplayConfiguration(
    KMSDisplayConfiguration const& conf)
    : mg::DisplayConfiguration(),
      drm_fd{conf.drm_fd},
      card(conf.card),
      outputs{conf.outputs}
{
}

void mge::KMSDisplayConfiguration::for_each_card(
    std::function<void(DisplayConfigurationCard const&)> f) const
{
    f(card);
}

void mge::KMSDisplayConfiguration::for_each_output(
    std::function<void(DisplayConfigurationOutput const&)> f) const
{
    for (auto const& output : outputs)
        f(*output);
}

void mge::KMSDisplayConfiguration::for_each_output(
    std::function<void(UserDisplayConfigurationOutput&)> f)
{
    for (auto& output : outputs)
    {
        UserDisplayConfigurationOutput user(*output);
        f(user);
    }
}

std::unique_ptr<mg::DisplayConfiguration> mge::KMSDisplayConfiguration::clone() const
{
    return std::make_unique<KMSDisplayConfiguration>(*this);
}

uint32_t mge::KMSDisplayConfiguration::get_kms_connector_id(
    DisplayConfigurationOutputId id) const
{
    auto iter = find_output_with_id(id);

    if (iter == outputs.end())
    {
        BOOST_THROW_EXCEPTION(
            std::out_of_range("Failed to find DisplayConfigurationOutput with provided id"));
    }

    return id.as_value();
}

size_t mge::KMSDisplayConfiguration::get_kms_mode_index(
    DisplayConfigurationOutputId id,
    size_t conf_mode_index) const
{
    auto iter = find_output_with_id(id);

    if (iter == outputs.end() || conf_mode_index >= (*iter)->modes.size())
    {
        BOOST_THROW_EXCEPTION(
            std::out_of_range("Failed to find valid mode index for DisplayConfigurationOutput with provided id/mode_index"));
    }

    return conf_mode_index;
}
void mge::KMSDisplayConfiguration::update()
{
    mgk::DRMModeResources resources{drm_fd};

    for (auto const& connector : resources.connectors())
    {
        update_output(resources, connector, **find_output_with_id(DisplayConfigurationOutputId(connector->connector_id)));
    };
}

void mge::KMSDisplayConfiguration::for_each_output(
    std::function<void(kms::EGLOutput const&)> const& f) const
{
    for (auto const& output : outputs)
    {
        f(*output);
    }
}

void mge::KMSDisplayConfiguration::update_output(
    mgk::DRMModeResources const& resources,
    mgk::DRMModeConnectorUPtr const& connector,
    kms::EGLOutput& output)
{
    output.physical_size_mm = geom::Size{connector->mmWidth, connector->mmHeight};
    output.connected = connector->connection == DRM_MODE_CONNECTED;

    uint32_t const invalid_mode_index = std::numeric_limits<uint32_t>::max();

    output.modes.clear();
    output.current_mode_index = invalid_mode_index;

    drmModeModeInfo* current_mode_info{nullptr};
    mgk::DRMModeCrtcUPtr current_crtc;
    if (connector->encoder_id)
    {
        auto encoder = resources.encoder(connector->encoder_id);
        if (encoder->crtc_id)
        {
            current_crtc = resources.crtc(encoder->crtc_id);
            current_mode_info = &current_crtc->mode;
        }
    }

    /* Add all the available modes and find the current and preferred one */
    for (int m = 0; m < connector->count_modes; m++)
    {
        drmModeModeInfo& mode_info = connector->modes[m];

        geom::Size size{mode_info.hdisplay, mode_info.vdisplay};

        double vrefresh_hz = calculate_vrefresh_hz(mode_info);

        output.modes.push_back({size, vrefresh_hz});

        if (current_mode_info && kms_modes_are_equal(mode_info, *current_mode_info))
            output.current_mode_index = m;

        if ((mode_info.type & DRM_MODE_TYPE_PREFERRED) == DRM_MODE_TYPE_PREFERRED)
            output.preferred_mode_index = m;
    }

    if (output.modes.empty())
    {
        output.preferred_mode_index = invalid_mode_index;
    }
    output.physical_size_mm = geom::Size{connector->mmWidth, connector->mmHeight};
}

std::vector<std::shared_ptr<mge::kms::EGLOutput>>::iterator
mge::KMSDisplayConfiguration::find_output_with_id(mg::DisplayConfigurationOutputId id)
{
    return std::find_if(
        outputs.begin(), outputs.end(),
        [id](std::shared_ptr<DisplayConfigurationOutput> const& output)
        {
            return output->id == id;
        });
}

std::vector<std::shared_ptr<mge::kms::EGLOutput>>::const_iterator
mge::KMSDisplayConfiguration::find_output_with_id(mg::DisplayConfigurationOutputId id) const
{
    return std::find_if(
        outputs.begin(), outputs.end(),
        [id](std::shared_ptr<DisplayConfigurationOutput> const& output)
        {
            return output->id == id;
        });
}
