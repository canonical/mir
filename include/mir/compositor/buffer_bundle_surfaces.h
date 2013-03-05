/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
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
class BufferProperties;
class BufferSwapper;

class BufferBundleSurfaces : public surfaces::BufferBundle
{
public:
    BufferBundleSurfaces(std::unique_ptr<BufferSwapper>&& swapper);

    BufferBundleSurfaces(
        std::unique_ptr<BufferSwapper>&& swapper, BufferProperties const& buffer_properties);

    ~BufferBundleSurfaces();

    std::shared_ptr<Buffer> secure_client_buffer();

    std::shared_ptr<surfaces::GraphicRegion> lock_back_buffer();

    geometry::PixelFormat get_bundle_pixel_format();
    geometry::Size bundle_size();
    void shutdown();

protected:
    BufferBundleSurfaces(const BufferBundleSurfaces&) = delete;
    BufferBundleSurfaces& operator=(const BufferBundleSurfaces&) = delete;

private:
    std::shared_ptr<BufferSwapper> swapper;
    std::map<Buffer*, BufferID> buffer_to_id_map;
    geometry::Size size;
    geometry::PixelFormat pixel_format;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_BUNDLE_SURFACES_H_ */
