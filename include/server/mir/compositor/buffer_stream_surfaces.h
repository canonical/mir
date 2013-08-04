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

#ifndef MIR_COMPOSITOR_BUFFER_STREAM_SURFACES_H_
#define MIR_COMPOSITOR_BUFFER_STREAM_SURFACES_H_

#include "mir/surfaces/buffer_stream.h"

#include <mutex>

namespace mir
{
namespace compositor
{

class BufferIDUniqueGenerator;
class BufferBundle;
class BackBufferStrategy;

class BufferStreamSurfaces : public surfaces::BufferStream
{
public:
    BufferStreamSurfaces(std::shared_ptr<BufferBundle> const& swapper);
    ~BufferStreamSurfaces();

    std::shared_ptr<graphics::Buffer> secure_client_buffer();

    std::shared_ptr<graphics::Buffer> lock_compositor_buffer() override;
    std::shared_ptr<graphics::Buffer> lock_snapshot_buffer() override;

    geometry::PixelFormat get_stream_pixel_format();
    geometry::Size stream_size();
    void allow_framedropping(bool);
    void force_requests_to_complete();

protected:
    BufferStreamSurfaces(const BufferStreamSurfaces&) = delete;
    BufferStreamSurfaces& operator=(const BufferStreamSurfaces&) = delete;

private:
    std::shared_ptr<BufferBundle> const buffer_bundle;
    std::shared_ptr<BackBufferStrategy> const back_buffer_strategy;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_STREAM_SURFACES_H_ */
