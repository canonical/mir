/*
 * Copyright © Canonical Ltd.
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
 */

#include <algorithm>
#include "real_kms_output_container.h"
#include "real_kms_output.h"
#include "kms-utils/drm_mode_resources.h"

namespace mgg = mir::graphics::gbm;

mgg::RealKMSOutputContainer::RealKMSOutputContainer(
    mir::Fd drm_fd,
    std::shared_ptr<PageFlipper> page_flipper)
    : drm_fd{std::move(drm_fd)},
      page_flipper{std::move(page_flipper)}
{
}

void mgg::RealKMSOutputContainer::for_each_output(std::function<void(std::shared_ptr<KMSOutput> const&)> functor) const
{
    for(auto& output: outputs)
        functor(output);
}

void mgg::RealKMSOutputContainer::update_from_hardware_state()
{
    decltype(outputs) new_outputs;

    auto const resources = std::make_unique<kms::DRMModeResources>(drm_fd);

    for (auto &&connector : resources->connectors())
    {
        // Caution: O(n²) here, but n is the number of outputs, so should
        // conservatively be << 100.
        auto existing_output = std::find_if(
            outputs.begin(),
            outputs.end(),
            [&connector, this](auto const &candidate)
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
                page_flipper));
        }
    }

    outputs = new_outputs;
}
