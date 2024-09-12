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

#ifndef MIR_GRAPHICS_GBM_KMS_OUTPUT_CONTAINER_H_
#define MIR_GRAPHICS_GBM_KMS_OUTPUT_CONTAINER_H_

#include <memory>
#include <functional>

namespace mir
{
namespace graphics
{
namespace atomic
{

class KMSOutput;

class KMSOutputContainer
{
public:
    virtual ~KMSOutputContainer() = default;

    virtual void for_each_output(std::function<void(std::shared_ptr<KMSOutput> const&)> functor) const = 0;

    /**
     * Re-probe hardware state and update output list.
     */
    virtual void update_from_hardware_state() = 0;
protected:
    KMSOutputContainer() = default;
    KMSOutputContainer(KMSOutputContainer const&) = delete;
    KMSOutputContainer& operator=(KMSOutputContainer const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_KMS_OUTPUT_CONTAINER_H_ */
