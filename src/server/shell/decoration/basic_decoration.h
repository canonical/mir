/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_SHELL_DECORATION_BASIC_DECORATION_H_
#define MIR_SHELL_DECORATION_BASIC_DECORATION_H_

#include "decoration.h"

#include "mir/geometry/rectangle.h"
#include "mir_toolkit/common.h"

#include <memory>
#include <vector>
#include <chrono>

namespace mir
{
class Executor;
namespace scene
{
class Surface;
class Session;
class SurfaceCreationParameters;
}
namespace input
{
class CursorImages;
}
namespace graphics
{
class GraphicBufferAllocator;
}
namespace frontend
{
class BufferStream;
}
namespace shell
{
class Shell;
class DefaultDecorationGeometry;
class StreamSpecification;
namespace decoration
{
template<typename T> class ThreadsafeAccess;
class StaticGeometry;
class WindowState;
class WindowSurfaceObserverManager;
class InputManager;
class InputState;
class Renderer;

class BasicDecoration
    : public Decoration
{
public:
    BasicDecoration(
        std::shared_ptr<shell::Shell> const& shell,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<Executor> const& executor,
        std::shared_ptr<input::CursorImages> const& cursor_images,
        std::shared_ptr<scene::Surface> const& window_surface);
    ~BasicDecoration();

    void window_state_updated();
    void input_state_updated();

    void request_move(std::chrono::nanoseconds const& timestamp);
    void request_resize(std::chrono::nanoseconds const& timestamp, MirResizeEdge edge);
    void request_toggle_maximize();
    void request_minimize();
    void request_close();
    void set_cursor(std::string const& cursor_image_name);

protected:
    /// Creates an up-to-date WindowState object
    auto new_window_state() const -> std::unique_ptr<WindowState>;

    /// Returns paramaters to create the decoration surface
    auto creation_params() const -> std::unique_ptr<scene::SurfaceCreationParameters>;

    /// Draw the decoration buffers and submit them to the surface
    /// Current states are stored int window_state and input_state members
    void update();

    struct DecorationSurfaceObserver;

    std::shared_ptr<ThreadsafeAccess<BasicDecoration>> threadsafe_self;
    std::unique_ptr<StaticGeometry const> const static_geometry;
    std::unique_ptr<WindowState const> window_state;
    std::unique_ptr<InputState const> input_state;

    std::shared_ptr<shell::Shell> const shell;
    std::shared_ptr<graphics::GraphicBufferAllocator> const buffer_allocator;
    std::shared_ptr<input::CursorImages> const cursor_images;
    std::shared_ptr<scene::Session> const session;

    class BufferStreams;
    std::unique_ptr<BufferStreams> buffer_streams;
    std::unique_ptr<Renderer> renderer;

    std::shared_ptr<scene::Surface> const window_surface;
    std::shared_ptr<scene::Surface> const decoration_surface;
    std::unique_ptr<WindowSurfaceObserverManager> const window_surface_observer_manager;
    std::unique_ptr<InputManager> const input_manager;
};
}
}
}

#endif // MIR_SHELL_DECORATION_BASIC_DECORATION_H_
