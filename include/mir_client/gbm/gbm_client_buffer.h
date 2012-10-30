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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_GBM_GBM_CLIENT_BUFFER_H_
#define MIR_CLIENT_GBM_GBM_CLIENT_BUFFER_H_

#include "mir_client/client_buffer.h"
#include "mir_client/mir_client_library.h"
#include "mir/geometry/rectangle.h"

#include <memory>

namespace mir
{
namespace client
{

class GBMClientBuffer : public ClientBuffer
{
public:
    GBMClientBuffer(std::shared_ptr<MirBufferPackage> && ,
                    geometry::Size size,
                    geometry::PixelFormat pf );
    
    std::shared_ptr<MemoryRegion> secure_for_cpu_write();
    geometry::Size size() const;
    geometry::PixelFormat pixel_format() const;
    std::shared_ptr<MirBufferPackage> get_buffer_package() const;

    GBMClientBuffer(const GBMClientBuffer&) = delete;
    GBMClientBuffer& operator=(const GBMClientBuffer&) = delete;

private:
    std::shared_ptr<MirBufferPackage> creation_package;
    const geometry::Rectangle rect;
    const geometry::PixelFormat buffer_pf;
};

}
}
#endif /* MIR_CLIENT_GBM_GBM_CLIENT_BUFFER_H_ */
