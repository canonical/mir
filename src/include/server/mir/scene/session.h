/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.   If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_SCENE_SESSION_H_
#define MIR_SCENE_SESSION_H_

#include "mir/fd.h"

#include <vector>
#include <sys/types.h>
#include <memory>
#include <string>

namespace mir
{
class ClientVisibleError;
class Executor;
namespace compositor
{
class BufferStream;
}
namespace wayland
{
template<typename>
class Weak;
}
namespace frontend
{
class WlSurface;
class BufferStream;
}
namespace shell
{
struct StreamSpecification;
struct SurfaceSpecification;
}
namespace graphics
{
struct BufferProperties;
}
namespace scene
{
class Surface;
class SurfaceObserver;

/// A single connection to a client application
/// Every mirclient session and wl_client maps to a scene::Session
class Session
{
public:
    virtual ~Session() = default;

    virtual auto process_id() const -> pid_t = 0;
    virtual auto socket_fd() const -> Fd = 0;
    virtual auto name() const -> std::string = 0;

    virtual void send_error(ClientVisibleError const&) = 0;

    virtual auto default_surface() const -> std::shared_ptr<Surface> = 0;

    virtual void hide() = 0;
    virtual void show() = 0;

    virtual void start_prompt_session() = 0;
    virtual void stop_prompt_session() = 0;
    virtual void suspend_prompt_session() = 0;
    virtual void resume_prompt_session() = 0;

    /// \param session can be nullptr (useful for testing). If not nullptr, must be equal to this. Must be passed in
    ///                because std::enable_shared_from_this causes trouble
    /// \param params data used to create the surface
    /// \param observer automatically added to surface after creation
    /// \return a newly created surface
    virtual auto create_surface(
        std::shared_ptr<Session> const& session,
        wayland::Weak<frontend::WlSurface> const& wayland_surface,
        shell::SurfaceSpecification const& params,
        std::shared_ptr<scene::SurfaceObserver> const& observer,
        Executor* observer_executor) -> std::shared_ptr<Surface> = 0;
    virtual void destroy_surface(std::shared_ptr<Surface> const& surface) = 0;
    virtual auto surface_after(std::shared_ptr<Surface> const& surface) const -> std::shared_ptr<Surface> = 0;

    virtual auto create_buffer_stream(graphics::BufferProperties const& props)
        -> std::shared_ptr<compositor::BufferStream> = 0;
    virtual void destroy_buffer_stream(std::shared_ptr<frontend::BufferStream> const& stream) = 0;
    virtual void configure_streams(Surface& surface, std::vector<shell::StreamSpecification> const& config) = 0;

protected:
    Session() = default;
    Session(Session const&) = delete;
    Session& operator=(Session const&) = delete;
};
}
}

#endif // MIR_SCENE_SESSION_H_
