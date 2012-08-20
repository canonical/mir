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

#include "mir/compositor/buffer.h"
#include "mir_platform/graphic_alloc_adaptor.h"

#include <hardware/gralloc.h>
#include <stdexcept>
#include <memory>

namespace mc=mir::compositor;
namespace geom=mir::geometry;

namespace mir
{
namespace graphics
{

class AndroidBuffer: public mc::Buffer
{
public:
    explicit AndroidBuffer(std::shared_ptr<GraphicAllocAdaptor> device, geom::Width w, geom::Height h, mc::PixelFormat pf); 
    ~AndroidBuffer(); 

    geom::Width width() const;

    geom::Height height() const;

    geom::Stride stride() const;

    mc::PixelFormat pixel_format() const;

    void lock();

    void unlock();

    Texture* bind_to_texture();

private:
    int convert_to_android_format(mc::PixelFormat);

    const geom::Width  buffer_width;
    const geom::Height buffer_height;
    const mc::PixelFormat buffer_format;
    geom::Stride buffer_stride;

    std::shared_ptr<GraphicAllocAdaptor> alloc_device;

    buffer_handle_t android_handle;
};

}
}
