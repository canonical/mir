/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "kms_connector.h"

#include <boost/throw_exception.hpp>
#include <algorithm>

namespace mgk = mir::graphics::kms;

namespace
{
bool encoder_is_used(mgk::DRMModeResources const& resources, uint32_t encoder_id)
{
    if (encoder_id == 0)
    {
        BOOST_THROW_EXCEPTION(std::invalid_argument{"Attempted to query an encoder with invalid ID 0"});
    }

    for (auto const& connector : resources.connectors())
    {
        if (connector->encoder_id == encoder_id &&
            connector->connection == DRM_MODE_CONNECTED)
        {
            auto encoder = resources.encoder(connector->encoder_id);
            if (encoder->crtc_id)
            {
                return true;
            }
        }
    }

    return false;
}

bool crtc_is_used(mgk::DRMModeResources const& resources, uint32_t crtc_id)
{

    for (auto const& connector : resources.connectors())
    {
        if (connector->connection == DRM_MODE_CONNECTED)
        {
            if (connector->encoder_id)
            {
                auto encoder = resources.encoder(connector->encoder_id);
                if (encoder->crtc_id == crtc_id)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

std::vector<mgk::DRMModeEncoderUPtr>
connector_available_encoders(mgk::DRMModeResources const& resources,
    drmModeConnector const* connector)
{
    std::vector<mgk::DRMModeEncoderUPtr> encoders;

    for (int i = 0; i < connector->count_encoders; i++)
    {
        if (!encoder_is_used(resources, connector->encoders[i]))
            encoders.push_back(resources.encoder(connector->encoders[i]));
    }

    return encoders;
}

bool encoder_supports_crtc_index(drmModeEncoder const* encoder, uint32_t crtc_index)
{
    return (encoder->possible_crtcs & (1 << crtc_index));
}

const char *connector_type_name(uint32_t type)
{
    static const int nnames = 15;
    static const char * const names[nnames] =
        {   // Ordered according to xf86drmMode.h
            "Unknown",
            "VGA",
            "DVII",
            "DVID",
            "DVIA",
            "Composite",
            "SVIDEO",
            "LVDS",
            "Component",
            "9PinDIN",
            "DisplayPort",
            "HDMIA",
            "HDMIB",
            "TV",
            "eDP"
        };

    if (type >= nnames)
        type = 0;

    return names[type];
}

}

std::string mgk::connector_name(mgk::DRMModeConnectorUPtr const& connector)
{
    std::string name = connector_type_name(connector->connector_type);
    name += '-';
    name += std::to_string(connector->connector_type_id);
    return name;
}

mgk::DRMModeCrtcUPtr mgk::find_crtc_for_connector(int drm_fd, mgk::DRMModeConnectorUPtr const& connector)
{
    mgk::DRMModeResources resources{drm_fd};

    /* Check to see if there is a crtc already connected */
    if (connector->encoder_id)
    {
        auto encoder = resources.encoder(connector->encoder_id);
        if (encoder->crtc_id)
        {
            return resources.crtc(encoder->crtc_id);
        }
    }

    auto available_encoders = connector_available_encoders(resources, connector.get());

    int crtc_index = 0;

    for (auto& crtc : resources.crtcs())
    {
        if (!crtc_is_used(resources, crtc->crtc_id))
        {
            for (auto& enc : available_encoders)
            {
                if (encoder_supports_crtc_index(enc.get(), crtc_index))
                {
                    return std::move(crtc);
                }
            }
        }
        crtc_index++;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to find CRTC"});
}
