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

#ifndef MIR_GRAPHICS_GBM_REAL_KMS_OUTPUT_CONTAINER_H_
#define MIR_GRAPHICS_GBM_REAL_KMS_OUTPUT_CONTAINER_H_

#include "kms_output_container.h"
#include "mir/fd.h"
#include <vector>

namespace mir
{
namespace graphics
{
namespace gbm
{

class PageFlipper;

class RealKMSOutputContainer : public KMSOutputContainer
{
public:
    RealKMSOutputContainer(
        mir::Fd drm_fd,
        std::shared_ptr<PageFlipper> page_flipper);

    void for_each_output(std::function<void(std::shared_ptr<KMSOutput> const&)> functor) const override;

    void update_from_hardware_state() override;
private:
    mir::Fd const drm_fd;
    std::vector<std::shared_ptr<KMSOutput>> outputs;
    std::shared_ptr<PageFlipper> const page_flipper;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_REAL_KMS_OUTPUT_CONTAINER_H_ */
