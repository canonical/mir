/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_PLATFORM_GBM_GBM_BUFFER_ALLOCATOR_H_
#define MIR_PLATFORM_GBM_GBM_BUFFER_ALLOCATOR_H_

#include "mir/compositor/graphic_buffer_allocator.h"

#include <gbm.h>

#include <memory>

namespace mir
{
namespace graphics
{
namespace gbm
{

class DeviceFactory
{
public:
    virtual std::shared_ptr<gbm_device> create_device() = 0;

protected:
    DeviceFactory() = default;
    DeviceFactory(const DeviceFactory&) = delete;
    virtual ~DeviceFactory() {}
    DeviceFactory& operator=(const DeviceFactory&) = delete;
    
};

class DrmDeviceFactory : public DeviceFactory
{
public:
    std::shared_ptr<gbm_device> create_device();
};

class GBMBufferAllocator: public compositor::GraphicBufferAllocator
{
public:
    GBMBufferAllocator(const std::shared_ptr<DeviceFactory>& factory);

    virtual std::unique_ptr<compositor::Buffer> alloc_buffer(
        geometry::Width w, geometry::Height h, compositor::PixelFormat pf);


private:
    std::shared_ptr<gbm_device> dev;
};

}
}
}

#endif // MIR_PLATFORM_GBM_GBM_BUFFER_ALLOCATOR_H_
