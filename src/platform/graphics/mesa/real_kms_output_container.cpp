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

#include "real_kms_output_container.h"
#include "real_kms_output.h"

namespace mgm = mir::graphics::mesa;

mgm::RealKMSOutputContainer::RealKMSOutputContainer(
    int drm_fd, std::shared_ptr<PageFlipper> const& page_flipper)
    : drm_fd{drm_fd},
      page_flipper{page_flipper}
{
}

std::shared_ptr<mgm::KMSOutput>
mgm::RealKMSOutputContainer::get_kms_output_for(uint32_t connector_id)
{
    std::shared_ptr<KMSOutput> output;

    auto output_iter = outputs.find(connector_id);
    if (output_iter == outputs.end())
    {
        output = std::make_shared<RealKMSOutput>(drm_fd, connector_id, page_flipper);
        outputs[connector_id] = output;
    }
    else
    {
        output = output_iter->second;
    }

    return output;
}

void mgm::RealKMSOutputContainer::for_each_output(std::function<void(KMSOutput&)> functor) const
{
    for(auto& pair: outputs)
        functor(*pair.second);
}
