/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "crtc_assignment.h"
#include "kms_output.h"
#include "real_kms_display_configuration.h"

#include <mir/graphics/kms/kms_connector.h>
#include <mir/log.h>

#include <boost/throw_exception.hpp>

#include <algorithm>
#include <drm_mode.h>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

namespace mga = mir::graphics::atomic;
namespace mg = mir::graphics;
namespace mgk = mir::graphics::kms;

namespace
{
struct CrtcInfo
{
    uint32_t id;
    uint32_t index;
};

struct Triple
{
    uint32_t encoder_id;
    uint32_t crtc_id;
    uint32_t plane_id;
};

struct PrimaryPlaneForCrtc
{
    uint32_t plane_id;
    uint32_t crtc_index;
};

auto primary_planes_for_crtcs(mir::Fd const& drm_fd, std::vector<CrtcInfo> const& crtcs)
    -> std::vector<PrimaryPlaneForCrtc>
{
    std::vector<PrimaryPlaneForCrtc> plane_crtcs;
    mgk::PlaneResources plane_resources{drm_fd};

    for (auto const& plane : plane_resources.planes())
    {
        mgk::ObjectProperties props{drm_fd, plane};
        if (static_cast<uint32_t>(props["type"]) != DRM_PLANE_TYPE_PRIMARY)
        {
            continue;
        }

        for (auto const& crtc : crtcs)
        {
            if (plane->possible_crtcs & (1u << crtc.index))
            {
                plane_crtcs.push_back({plane->plane_id, crtc.index});
            }
        }
    }

    return plane_crtcs;
}

auto compatible_triples(
    mgk::DRMModeResources const& resources,
    std::vector<CrtcInfo> const& crtcs,
    std::vector<PrimaryPlaneForCrtc> const& primary_planes,
    mgk::DRMModeConnectorUPtr const& connector) -> std::vector<Triple>
{
    std::vector<Triple> triples;

    for (int i = 0; i < connector->count_encoders; ++i)
    {
        auto const encoder_id = connector->encoders[i];
        auto encoder = resources.encoder(encoder_id);

        for (auto const& crtc : crtcs)
        {
            if (!(encoder->possible_crtcs & (1u << crtc.index)))
            {
                continue;
            }

            for (auto const& primary_plane : primary_planes)
            {
                if (primary_plane.crtc_index == crtc.index)
                {
                    triples.push_back({encoder_id, crtc.id, primary_plane.plane_id});
                }
            }
        }
    }

    return triples;
}

}

mga::CrtcAssignment::CrtcAssignment(mgk::DRMModeCrtcUPtr crtc, mgk::DRMModePlaneUPtr plane) :
    crtc{std::move(crtc)},
    plane{std::move(plane)}
{}

auto mga::solve_crtc_assignments(mir::Fd const& drm_fd, RealKMSDisplayConfiguration const& conf)
    -> std::map<mg::DisplayConfigurationOutputId, CrtcAssignment>
{
    mgk::DRMModeResources resources{drm_fd};

    std::vector<CrtcInfo> crtcs;
    {
        uint32_t index = 0;
        for (auto const& crtc : resources.crtcs())
        {
            crtcs.push_back({crtc->crtc_id, index});
            ++index;
        }
    }

    auto const primary_planes = primary_planes_for_crtcs(drm_fd, crtcs);

    struct OutputOptions
    {
        mg::DisplayConfigurationOutputId id;
        uint32_t connector_id;
        std::vector<Triple> triples;
    };

    std::vector<OutputOptions> active_outputs;

    conf.for_each_output(
        [&](mg::DisplayConfigurationOutput const& output)
        {
            if (!is_active_physical_output(output))
            {
                return;
            }

            auto kms_output = conf.get_output_for(output.id);
            auto connector = resources.connector(kms_output->id());

            active_outputs.push_back(
                {output.id, kms_output->id(), compatible_triples(resources, crtcs, primary_planes, connector)});
        });

    std::ranges::sort(
        active_outputs, [](auto const& lhs, auto const& rhs) { return lhs.triples.size() < rhs.triples.size(); });

    std::set<uint32_t> used_encoders;
    std::set<uint32_t> used_crtcs;
    std::set<uint32_t> used_planes;

    std::map<mg::DisplayConfigurationOutputId, CrtcAssignment> result;

    for (auto const& output : active_outputs)
    {
        auto chosen = std::ranges::find_if(
            output.triples,
            [&](auto const& triple)
            {
                return !used_encoders.contains(triple.encoder_id) && !used_crtcs.contains(triple.crtc_id) &&
                       !used_planes.contains(triple.plane_id);
            });

        if (chosen == output.triples.end())
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error(
                    "Failed to find a compatible CRTC/encoder/plane assignment for output " +
                    std::to_string(output.id.as_value())));
        }

        auto crtc = mgk::get_crtc(drm_fd, chosen->crtc_id);
        auto plane = mgk::get_plane(drm_fd, chosen->plane_id);

        if (!crtc || !plane)
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error(
                    "Failed to retrieve assigned CRTC or plane for output " + std::to_string(output.id.as_value())));
        }

        result.emplace(output.id, CrtcAssignment{std::move(crtc), std::move(plane)});

        used_encoders.insert(chosen->encoder_id);
        used_crtcs.insert(chosen->crtc_id);
        used_planes.insert(chosen->plane_id);
    }

    return result;
}
