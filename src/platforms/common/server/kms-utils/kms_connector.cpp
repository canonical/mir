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

namespace mgk = mir::graphics::kms;
auto try_find_crtc_and_plane_for_connector(
    mgk::DRMModeConnectorUPtr const& connector,
    int drm_fd,
    mgk::PlaneResources& plane_res,
    mgk::DRMModeResources& resources) -> std::optional<std::pair<mgk::DRMModeCrtcUPtr, mgk::DRMModePlaneUPtr>>
{
    mgk::DRMModeCrtcUPtr crtc;
    int crtc_index{-1};

    /* If there's already a CRTC connected, find it */
    if (connector->encoder_id)
    {
        auto encoder = mgk::get_encoder(drm_fd, connector->encoder_id);
        if (encoder->crtc_id)
        {
            /* There's already a CRTC connected; we only need to find its index */
            auto our_crtc = std::find_if(
                resources.crtcs().begin(),
                resources.crtcs().end(),
                [crtc_id = encoder->crtc_id](mgk::DRMModeCrtcUPtr& crtc) { return crtc_id == crtc->crtc_id; });
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

    if (crtc)
    {
        for (auto& plane : plane_res.planes())
        {
            mgk::ObjectProperties plane_props{drm_fd, plane->plane_id, DRM_MODE_OBJECT_PLANE};
            if (plane_props["type"] != DRM_PLANE_TYPE_PRIMARY)
                continue;

            if (plane->possible_crtcs & (1 << crtc_index))
            {
                return std::make_pair(std::move(crtc), std::move(plane));
            }
        }
    }

    return std::nullopt;
}

auto try_find_any_crtc_and_plane_for_connector(
    mgk::DRMModeConnectorUPtr const& connector,
    int drm_fd,
    mgk::PlaneResources& plane_res,
    mgk::DRMModeResources& resources) -> std::optional<std::pair<mgk::DRMModeCrtcUPtr, mgk::DRMModePlaneUPtr>>
{
    auto const encoders = connector_available_encoders(resources, connector.get());
    std::vector const crtcs{resources.crtcs().begin(), resources.crtcs().end()};
    for (auto& plane : plane_res.planes())
    {
        mgk::ObjectProperties plane_props{drm_fd, plane->plane_id, DRM_MODE_OBJECT_PLANE};
        if (plane_props["type"] != DRM_PLANE_TYPE_PRIMARY)
            continue;

        for (auto const& enc : encoders)
        {
            auto const joint_crtc_mask = enc->possible_crtcs & plane->possible_crtcs;
            if (joint_crtc_mask)
            {
                for (auto i = 0ul; i < 32; i++)
                {
                    if (joint_crtc_mask >> i == 0 || crtc_is_used(resources, (*crtcs[i])->crtc_id))
                        continue;

                    return std::make_pair(std::move(*crtcs[i]), std::move(plane));
                }
            }
        }
    }

    return std::nullopt;
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

    return *try_find_crtc_and_plane_for_connector(connector, drm_fd, plane_res, resources)
        .or_else([&] { return try_find_any_crtc_and_plane_for_connector(connector, drm_fd, plane_res, resources); })
        .or_else(
            [] -> std::optional<std::pair<DRMModeCrtcUPtr, DRMModePlaneUPtr>>
            {
                BOOST_THROW_EXCEPTION(std::runtime_error{"Could not find primary plane for CRTC"});
            });
}
