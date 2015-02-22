/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "real_kms_display_configuration.h"
#include "drm_mode_resources.h"
#include "mir/graphics/pixel_format_utils.h"

#include <cmath>
#include <limits>

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <algorithm>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
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

}

mgm::RealKMSDisplayConfiguration::RealKMSDisplayConfiguration(int drm_fd)
    : drm_fd{drm_fd}
{
    update();
}

mgm::RealKMSDisplayConfiguration::RealKMSDisplayConfiguration(
    RealKMSDisplayConfiguration const& conf)
    : KMSDisplayConfiguration(), drm_fd{conf.drm_fd},
      card(conf.card), outputs{conf.outputs}
{
}

mgm::RealKMSDisplayConfiguration& mgm::RealKMSDisplayConfiguration::operator=(
    RealKMSDisplayConfiguration const& conf)
{
    if (&conf != this)
    {
        drm_fd = conf.drm_fd;
        card = conf.card;
        outputs = conf.outputs;
    }

    return *this;
}

void mgm::RealKMSDisplayConfiguration::for_each_card(
    std::function<void(DisplayConfigurationCard const&)> f) const
{
    f(card);
}

void mgm::RealKMSDisplayConfiguration::for_each_output(
    std::function<void(DisplayConfigurationOutput const&)> f) const
{
    for (auto const& output : outputs)
        f(output);
}

void mgm::RealKMSDisplayConfiguration::for_each_output(
    std::function<void(UserDisplayConfigurationOutput&)> f)
{
    for (auto& output : outputs)
    {
        UserDisplayConfigurationOutput user(output);
        f(user);
    }
}

uint32_t mgm::RealKMSDisplayConfiguration::get_kms_connector_id(
    DisplayConfigurationOutputId id) const
{
    auto iter = find_output_with_id(id);

    if (iter == outputs.end())
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to find DisplayConfigurationOutput with provided id"));
    }

    return id.as_value();
}

size_t mgm::RealKMSDisplayConfiguration::get_kms_mode_index(
    DisplayConfigurationOutputId id,
    size_t conf_mode_index) const
{
    auto iter = find_output_with_id(id);

    if (iter == outputs.end() || conf_mode_index >= iter->modes.size())
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to find valid mode index for DisplayConfigurationOutput with provided id/mode_index"));
    }

    return conf_mode_index;
}
void mgm::RealKMSDisplayConfiguration::update()
{
    DRMModeResources resources{drm_fd};

    size_t max_outputs = std::min(resources.num_crtcs(), resources.num_connectors());
    card = {DisplayConfigurationCardId{0}, max_outputs};

    resources.for_each_connector([&](DRMModeConnectorUPtr connector)
    {
        add_or_update_output(resources, *connector);
    });
}

void mgm::RealKMSDisplayConfiguration::add_or_update_output(
    DRMModeResources const& resources,
    drmModeConnector const& connector)
{
    DisplayConfigurationOutputId id{static_cast<int>(connector.connector_id)};
    DisplayConfigurationCardId card_id{0};
    DisplayConfigurationOutputType const type{
        kms_connector_type_to_output_type(connector.connector_type)};
    geom::Size physical_size{connector.mmWidth, connector.mmHeight};
    bool connected{connector.connection == DRM_MODE_CONNECTED};
    size_t const invalid_mode_index = std::numeric_limits<size_t>::max();
    size_t current_mode_index{invalid_mode_index};
    size_t preferred_mode_index{invalid_mode_index};
    std::vector<DisplayConfigurationMode> modes;
    std::vector<MirPixelFormat> formats {mir_pixel_format_argb_8888,
                                         mir_pixel_format_xrgb_8888};

    drmModeModeInfo current_mode_info = drmModeModeInfo();

    /* Get information about the current mode */
    auto encoder = resources.encoder(connector.encoder_id);
    if (encoder)
    {
        auto crtc = resources.crtc(encoder->crtc_id);
        if (crtc)
            current_mode_info = crtc->mode;
    }

    /* Add all the available modes and find the current and preferred one */
    for (int m = 0; m < connector.count_modes; m++)
    {
        drmModeModeInfo& mode_info = connector.modes[m];

        geom::Size size{mode_info.hdisplay, mode_info.vdisplay};

        double vrefresh_hz = calculate_vrefresh_hz(mode_info);

        modes.push_back({size, vrefresh_hz});

        if (kms_modes_are_equal(mode_info, current_mode_info))
            current_mode_index = m;

        if ((mode_info.type & DRM_MODE_TYPE_PREFERRED) == DRM_MODE_TYPE_PREFERRED)
            preferred_mode_index = m;
    }

    /* Add or update the output */
    auto iter = find_output_with_id(id);

    if (iter == outputs.end())
    {
        outputs.push_back({id, card_id, type, formats, modes, preferred_mode_index,
                           physical_size, connected, false, geom::Point(),
                           current_mode_index, mir_pixel_format_xrgb_8888,
                           mir_power_mode_on, mir_orientation_normal});
    }
    else
    {
        auto& output = *iter;

        if (current_mode_index != invalid_mode_index)
        {
            output.current_mode_index = current_mode_index;
        }
        else if (!modes.empty() &&  // If empty retain old current_mode_index!
                 ( output.current_mode_index >= modes.size() ||
                   output.modes[output.current_mode_index] !=
                          modes[output.current_mode_index]))
        {
            // current_mode_index is invalid and the definition of the old
            // current mode has also changed (different display plugged in)
            // so fall back to the preferred mode...
            output.current_mode_index = preferred_mode_index;
        }
        // else output.current_mode_index is correct and unchanged.

        output.modes = modes;
        output.preferred_mode_index = preferred_mode_index;
        output.physical_size_mm = physical_size;
        output.connected = connected;
        output.current_format = mir_pixel_format_xrgb_8888;
    }
}

std::vector<mg::DisplayConfigurationOutput>::iterator
mgm::RealKMSDisplayConfiguration::find_output_with_id(mg::DisplayConfigurationOutputId id)
{
    return std::find_if(outputs.begin(), outputs.end(),
                        [id](DisplayConfigurationOutput const& output)
                        {
                            return output.id == id;
                        });
}

std::vector<mg::DisplayConfigurationOutput>::const_iterator
mgm::RealKMSDisplayConfiguration::find_output_with_id(mg::DisplayConfigurationOutputId id) const
{
    return std::find_if(outputs.begin(), outputs.end(),
                        [id](DisplayConfigurationOutput const& output)
                        {
                            return output.id == id;
                        });
}

// Compatibility means conf1 can be attained from conf2 (and vice versa)
// without recreating the display buffers (e.g. conf1 and conf2 are identical
// except one of the outputs of conf1 is rotated w.r.t. that of conf2). If
// the two outputs differ in their power state, the display buffers would need
// to be allocated/destroyed, and hence should not be considered compatible.
bool mgm::compatible(mgm::RealKMSDisplayConfiguration const& conf1, mgm::RealKMSDisplayConfiguration const& conf2)
{
    bool compatible{(conf1.drm_fd         == conf2.drm_fd) &&
                    (conf1.card           == conf2.card)   &&
                    (conf1.outputs.size() == conf2.outputs.size())};

    if (compatible)
    {
        unsigned int const count = conf1.outputs.size();

        for (unsigned int i = 0; i < count; ++i)
        {
            compatible &= (conf1.outputs[i].power_mode == conf2.outputs[i].power_mode);
            if (compatible)
            {
                auto clone = conf2.outputs[i];

                // ignore difference in orientation
                clone.orientation = conf1.outputs[i].orientation;
                compatible &= (conf1.outputs[i] == clone);
            }
            else
            	break;
        }
    }

    return compatible;
}
