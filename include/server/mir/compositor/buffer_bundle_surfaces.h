/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_COMPOSITOR_BUFFER_BUNDLE_SURFACES_H_
#define MIR_COMPOSITOR_BUFFER_BUNDLE_SURFACES_H_

#include "mir/surfaces/buffer_bundle.h"

#include <map>

namespace mir
{
namespace compositor
{

class BufferIDUniqueGenerator;
struct BufferProperties;
class SwapperDirector;

class BufferStreamSurfaces : public surfaces::BufferBundle
{
public:
    BufferStreamSurfaces(std::shared_ptr<SwapperDirector> const& swapper);

    ~BufferStreamSurfaces();

    std::shared_ptr<Buffer> secure_client_buffer();

    std::shared_ptr<surfaces::GraphicRegion> lock_back_buffer();

    geometry::PixelFormat get_bundle_pixel_format();
    geometry::Size bundle_size();
    void shutdown();

protected:
    BufferStreamSurfaces(const BufferStreamSurfaces&) = delete;
    BufferStreamSurfaces& operator=(const BufferStreamSurfaces&) = delete;

private:
    std::shared_ptr<SwapperDirector> director;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_BUNDLE_SURFACES_H_ */
