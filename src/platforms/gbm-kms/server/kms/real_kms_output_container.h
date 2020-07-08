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

#ifndef MIR_GRAPHICS_MESA_REAL_KMS_OUTPUT_CONTAINER_H_
#define MIR_GRAPHICS_MESA_REAL_KMS_OUTPUT_CONTAINER_H_

#include "kms_output_container.h"
#include <vector>

namespace mir
{
namespace graphics
{
namespace mesa
{

class PageFlipper;

class RealKMSOutputContainer : public KMSOutputContainer
{
public:
    RealKMSOutputContainer(
        std::vector<int> const& drm_fds,
        std::function<std::shared_ptr<PageFlipper>(int drm_fd)> const& construct_page_flipper);

    void for_each_output(std::function<void(std::shared_ptr<KMSOutput> const&)> functor) const override;

    void update_from_hardware_state() override;
private:
    std::vector<int> const drm_fds;
    std::vector<std::shared_ptr<KMSOutput>> outputs;
    std::function<std::shared_ptr<PageFlipper>(int drm_fd)> const construct_page_flipper;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_REAL_KMS_OUTPUT_CONTAINER_H_ */
