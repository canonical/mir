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

#ifndef MIR_GRAPHICS_MESA_KMS_OUTPUT_CONTAINER_H_
#define MIR_GRAPHICS_MESA_KMS_OUTPUT_CONTAINER_H_

#include <cstdint>
#include <memory>

namespace mir
{
namespace graphics
{
namespace mesa
{

class KMSOutput;

class KMSOutputContainer
{
public:
    virtual ~KMSOutputContainer() = default;

    virtual std::shared_ptr<KMSOutput> get_kms_output_for(uint32_t connector_id) = 0;
    virtual void for_each_output(std::function<void(KMSOutput&)> functor) const = 0;

protected:
    KMSOutputContainer() = default;
    KMSOutputContainer(KMSOutputContainer const&) = delete;
    KMSOutputContainer& operator=(KMSOutputContainer const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_KMS_OUTPUT_CONTAINER_H_ */
