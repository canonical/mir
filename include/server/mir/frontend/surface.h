/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_FRONTEND_SURFACE_H_
#define MIR_FRONTEND_SURFACE_H_

#include "mir/geometry/pixel_format.h"
#include "mir/geometry/point.h"
#include "mir/geometry/size.h"
#include "mir_toolkit/common.h"

#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;
class InternalSurface;
}
namespace input
{
class InputChannel;
}

namespace frontend
{

class ClientBufferTracker;

class Surface
{
public:

    virtual ~Surface() {}

    virtual void force_requests_to_complete() = 0;

    virtual geometry::Size size() const = 0;
    virtual geometry::PixelFormat pixel_format() const = 0;

    /// Submit the current client buffer, return the next client buffer
    ///
    /// \param [out] need_ipc:  True if the buffer content must be sent via IPC
    ///                         False if only the buffer's ID must be sent.
    /// \returns                The next client buffer
    virtual std::shared_ptr<graphics::Buffer> advance_client_buffer(bool& need_full_ipc) = 0;

    virtual bool supports_input() const = 0;
    virtual int client_input_fd() const = 0;

    virtual int configure(MirSurfaceAttrib attrib, int value) = 0;

protected:
    Surface() = default;
    Surface(Surface const&) = delete;
    Surface& operator=(Surface const&) = delete;
};

auto as_internal_surface(std::shared_ptr<Surface> const& surface)
    -> std::shared_ptr<graphics::InternalSurface>;

class ClientTrackingSurface : public Surface
{
public:
    ClientTrackingSurface();
    virtual ~ClientTrackingSurface() = default;
    
    virtual std::shared_ptr<graphics::Buffer> advance_client_buffer(bool& need_full_ipc) override;

    virtual std::shared_ptr<graphics::Buffer> advance_client_buffer() = 0;
private:
    std::shared_ptr<ClientBufferTracker> client_tracker;
};

}   
}


#endif /* MIR_FRONTEND_SURFACE_H_ */
