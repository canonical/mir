/*
 * Copyright © 2012-2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored By: Robert Carr <racarr@canonical.com>
 *              William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_MIR_CLIENT_SESSION_H_
#define MIR_FRONTEND_MIR_CLIENT_SESSION_H_

#include "mir/frontend/surface_id.h"
#include "mir/frontend/buffer_stream_id.h"

#include <memory>

namespace mir
{
namespace shell
{
class Shell;
}
namespace scene
{
class Session;
struct SurfaceCreationParameters;
}
namespace graphics
{
struct BufferProperties;
}
namespace frontend
{
class Surface;
class BufferStream;
class EventSink;

/// Session interface specific to applications using the deprecated mirclient protocol
class MirClientSession
{
public:
    virtual ~MirClientSession() = default;

    virtual auto name() const -> std::string = 0;
    virtual auto get_surface(SurfaceId surface) const -> std::shared_ptr<Surface> = 0;

    virtual auto create_surface(
        std::shared_ptr<shell::Shell> const& shell,
        scene::SurfaceCreationParameters const& params,
        std::shared_ptr<EventSink> const& sink) -> SurfaceId = 0;
    virtual void destroy_surface(std::shared_ptr<shell::Shell> const& shell, SurfaceId surface) = 0;

    virtual auto create_buffer_stream(graphics::BufferProperties const& props) -> BufferStreamId = 0;
    virtual auto get_buffer_stream(BufferStreamId stream) const -> std::shared_ptr<BufferStream> = 0;
    virtual void destroy_buffer_stream(BufferStreamId stream) = 0;

protected:
    MirClientSession() = default;
    MirClientSession(MirClientSession const&) = delete;
    MirClientSession& operator=(MirClientSession const&) = delete;
};

}
}

#endif // MIR_FRONTEND_MIR_CLIENT_SESSION_H_
