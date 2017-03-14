/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
    int drm_fd, std::shared_ptr<PageFlipper> const& page_flipper)
    : drm_fd{drm_fd},
      page_flipper{page_flipper}
{
}

void mgm::RealKMSOutputContainer::for_each_output(std::function<void(std::shared_ptr<KMSOutput> const&)> functor) const
{
    for(auto& output: outputs)
        functor(output);
}

void mgm::RealKMSOutputContainer::update_from_hardware_state()
{
    kms::DRMModeResources resources{drm_fd};

    decltype(outputs) new_outputs;

    for (auto&& connector : resources.connectors())
    {
        auto existing_output = std::find_if(
            outputs.begin(),
            outputs.end(),
            [&connector](auto const& candidate)
            {
                return connector->connector_id == candidate->id();
            });

        if (existing_output != outputs.end())
        {
            new_outputs.push_back(std::move(*existing_output));
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
