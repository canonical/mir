/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include <algorithm>
#include "real_kms_output_container.h"
#include "real_kms_output.h"
#include "kms-utils/drm_mode_resources.h"

namespace mgm = mir::graphics::mesa;

mgm::RealKMSOutputContainer::RealKMSOutputContainer(
    std::vector<int> const& drm_fds,
    std::function<std::shared_ptr<PageFlipper>(int)> const& construct_page_flipper)
    : drm_fds{drm_fds},
      construct_page_flipper{construct_page_flipper}
{
}

void mgm::RealKMSOutputContainer::for_each_output(std::function<void(std::shared_ptr<KMSOutput> const&)> functor) const
{
    for(auto& output: outputs)
        functor(output);
}

void mgm::RealKMSOutputContainer::update_from_hardware_state()
{
    decltype(outputs) new_outputs;

    // TODO: Accumulate errors and present them all.
    std::exception_ptr last_error;

    for (auto drm_fd : drm_fds)
    {
        std::unique_ptr<kms::DRMModeResources> resources;
        try
        {
            resources = std::make_unique<kms::DRMModeResources>(drm_fd);
        }
        catch (std::exception const&)
        {
            last_error = std::current_exception();
            continue;
        }

        for (auto &&connector : resources->connectors())
        {
            // Caution: O(n²) here, but n is the number of outputs, so should
            // conservatively be << 100.
            auto existing_output = std::find_if(
                outputs.begin(),
                outputs.end(),
                [&connector, drm_fd](auto const &candidate)
                {
                    return
                        connector->connector_id == candidate->id() &&
                        drm_fd == candidate->drm_fd();
                });

            if (existing_output != outputs.end())
            {
                // We could drop this down to O(n) by being smarter about moving out
                // of the outputs vector.
                //
                // That's a bit of a faff, so just do the simple thing for now.
                new_outputs.push_back(*existing_output);
                new_outputs.back()->refresh_hardware_state();
            }
            else
            {
                new_outputs.push_back(std::make_shared<RealKMSOutput>(
                    drm_fd,
                    std::move(connector),
                    construct_page_flipper(drm_fd)));
            }
        }

    }
    if (new_outputs.empty() && last_error)
    {
        std::rethrow_exception(last_error);
    }

    outputs = new_outputs;
}
