/*
 * Copyright © Canonical Ltd.
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
 */

#include <mir/graphics/kms/kms_connector.h>
#include <mir/output_type_names.h>

#include <boost/throw_exception.hpp>
#include <algorithm>
#include <stdexcept>
#include <optional>
#include <ranges>

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

bool is_primary_plane(int drm_fd, mgk::DRMModePlaneUPtr const& plane)
{
    mgk::ObjectProperties plane_props{drm_fd, plane->plane_id, DRM_MODE_OBJECT_PLANE};
    return plane_props["type"] == DRM_PLANE_TYPE_PRIMARY;
}

bool plane_compatible_with_crtc(mgk::DRMModePlaneUPtr const& plane, uint32_t crtc_index)
{
    return plane->possible_crtcs & (1 << crtc_index);
}

std::optional<uint32_t> find_crtc_index(
    mgk::DRMModeResources const& resources,
    uint32_t crtc_id)
{
    auto const& crtcs = resources.crtcs();
    auto it = std::ranges::find_if(crtcs, [crtc_id](auto const& crtc) {
        return crtc->crtc_id == crtc_id;
    });
    
    if (it != crtcs.end())
    {
        return std::distance(crtcs.begin(), it);
    }
    return std::nullopt;
}

std::optional<mgk::DRMModePlaneUPtr> find_primary_plane_for_crtc(
    int drm_fd,
    mgk::PlaneResources& plane_res,
    uint32_t crtc_index)
{
    auto& planes = plane_res.planes();
    auto it = std::ranges::find_if(planes, [drm_fd, crtc_index](auto& plane) {
        return plane_compatible_with_crtc(plane, crtc_index) && is_primary_plane(drm_fd, plane);
    });
    
    if (it != planes.end())
    {
        return std::move(*it);
    }
    return std::nullopt;
}

std::optional<std::pair<mgk::DRMModeCrtcUPtr, mgk::DRMModePlaneUPtr>> find_compatible_crtc_plane(
    int drm_fd,
    mgk::DRMModeResources& resources,
    mgk::PlaneResources& plane_res,
    mgk::DRMModeConnectorUPtr const& connector)
{
    for (auto& plane : plane_res.planes())
    {
        if (!is_primary_plane(drm_fd, plane))
        {
            continue;
        }

        for (int i = 0; i < connector->count_encoders; i++)
        {
            auto encoder = resources.encoder(connector->encoders[i]);
            uint32_t compatible_mask = plane->possible_crtcs & encoder->possible_crtcs;

            uint32_t crtc_index = 0;
            for (auto& crtc : resources.crtcs())
            {
                if ((compatible_mask & (1 << crtc_index)) && !crtc_is_used(resources, crtc->crtc_id))
                {
                    return std::make_pair(std::move(crtc), std::move(plane));
                }
                ++crtc_index;
            }
        }
    }
    return std::nullopt;
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
    DRMModeResources resources{drm_fd};
    PlaneResources plane_res{drm_fd};

    /* If there's already a CRTC connected, find a compatible primary plane for it */
    if (connector->encoder_id)
    {
        auto encoder = resources.encoder(connector->encoder_id);
        if (encoder->crtc_id)
        {
            if (auto const crtc_index = find_crtc_index(resources, encoder->crtc_id))
            {
                if (auto plane = find_primary_plane_for_crtc(drm_fd, plane_res, *crtc_index))
                {
                    return std::make_pair(resources.crtc(encoder->crtc_id), std::move(*plane));
                }
                BOOST_THROW_EXCEPTION(std::runtime_error{"Could not find primary plane for existing CRTC"});
            }
            BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to find index of CRTC?!"});
        }
    }

    /* No existing CRTC, so find a compatible CRTC-plane combination */
    if (auto result = find_compatible_crtc_plane(drm_fd, resources, plane_res, connector))
    {
        return std::move(*result);
    }

    BOOST_THROW_EXCEPTION(std::runtime_error{"Could not find compatible CRTC and primary plane for connector"});
}
