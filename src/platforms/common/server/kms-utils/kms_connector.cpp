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

#include "kms_connector.h"
#include "mir/output_type_names.h"

#include <boost/throw_exception.hpp>
#include <algorithm>
#include <stdexcept>

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

}

std::string mgk::connector_name(mgk::DRMModeConnectorUPtr const& connector)
{
    std::string name = mir::output_type_name(connector->connector_type);
    name += '-';
    name += std::to_string(connector->connector_type_id);
    return name;
}

namespace
{
std::tuple<mgk::DRMModeCrtcUPtr, int> find_crtc_and_index_for_connector(
    mgk::DRMModeResources const& resources,
    mgk::DRMModeConnectorUPtr const& connector)
{
    int crtc_index = 0;

    auto const encoders = connector_available_encoders(resources, connector.get());

    for (auto& crtc : resources.crtcs())
    {
        if (!crtc_is_used(resources, crtc->crtc_id))
        {
            for (auto& enc : encoders)
            {
                if (encoder_supports_crtc_index(enc.get(), crtc_index))
                {
                    return std::tuple<mgk::DRMModeCrtcUPtr, int>{std::move(crtc), crtc_index};
                }
            }
        }
        crtc_index++;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to find CRTC"});
}
}

mgk::DRMModeCrtcUPtr mgk::find_crtc_for_connector(int drm_fd, mgk::DRMModeConnectorUPtr const& connector)
{
    mgk::DRMModeResources resources{drm_fd};

    /* If there is already a CRTC connected we can just return it */
    if (connector->encoder_id)
    {
        auto encoder = resources.encoder(connector->encoder_id);
        if (encoder->crtc_id)
        {
            return resources.crtc(encoder->crtc_id);
        }
    }

    return std::get<0>(find_crtc_and_index_for_connector(resources, connector));
}

auto mgk::find_crtc_with_primary_plane(
    int drm_fd,
    mgk::DRMModeConnectorUPtr const& connector) -> std::pair<DRMModeCrtcUPtr, DRMModePlaneUPtr>
{
    /*
     * TODO: This currently has a sequential find-crtc-then-find-primary-plane-for-it algorithm.
     * This is needlessly restrictive - it will fail if the first available CRTC found doesn't have
     * an appropriate primary plane, even if other CRTCs are available and do have an appropriate plane.
     */
    DRMModeCrtcUPtr crtc;

    DRMModeResources resources{drm_fd};
    int crtc_index{-1};

    /* If there's already a CRTC connected, find it */
    if (connector->encoder_id)
    {
        auto encoder = get_encoder(drm_fd, connector->encoder_id);
        if (encoder->crtc_id)
        {
            /* There's already a CRTC connected; we only need to find its index */
            auto our_crtc = std::find_if(
                resources.crtcs().begin(),
                resources.crtcs().end(),
            [crtc_id = encoder->crtc_id](mgk::DRMModeCrtcUPtr& crtc)
            {
                return crtc_id == crtc->crtc_id;
            });
            if (our_crtc == resources.crtcs().end())
            {
                BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to find index of CRTC?!"});
            }
            crtc = std::move(*our_crtc);
            crtc_index = std::distance(resources.crtcs().begin(), our_crtc);
        }
    }

    if (!crtc)
    {
        std::tie(crtc, crtc_index) = find_crtc_and_index_for_connector(resources, connector);
    }
    
    mgk::PlaneResources plane_res{drm_fd};

    for (auto& plane : plane_res.planes())
    {
        if (plane->possible_crtcs & (1 << crtc_index))
        {
            ObjectProperties plane_props{drm_fd, plane->plane_id, DRM_MODE_OBJECT_PLANE};
            if (plane_props["type"] == DRM_PLANE_TYPE_PRIMARY)
            {
                return std::make_pair(std::move(crtc), std::move(plane));
            }
        }
    }

    BOOST_THROW_EXCEPTION(std::runtime_error{"Could not find primary plane for CRTC"});
}
