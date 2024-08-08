/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_SHELL_DECORATION_BASIC_DECORATION_H_
#define MIR_SHELL_DECORATION_BASIC_DECORATION_H_

#include "decoration.h"

#include "mir/geometry/rectangle.h"
#include "mir_toolkit/common.h"

#include <memory>
#include <vector>
#include <chrono>
#include <optional>

struct MirInputEvent;

namespace mir
{
class Executor;
namespace scene
{
class Surface;
class Session;
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
        std::shared_ptr<scene::Surface> const& window_surface,
        float x11_scale);
    ~BasicDecoration();

    void window_state_updated();
    void input_state_updated();

    void request_move(MirInputEvent const* event);
    void request_resize(MirInputEvent const* event, MirResizeEdge edge);
    void request_toggle_maximize();
    void request_minimize();
    void request_close();
    void set_cursor(std::string const& cursor_image_name);

    void set_scale(float scale) override;

protected:
    /// Creates an up-to-date WindowState object
    auto new_window_state() const -> std::unique_ptr<WindowState>;

    /// Returns paramaters to create the decoration surface
    auto create_surface() const -> std::shared_ptr<scene::Surface>;

    /// Draw the decoration buffers and submit them to the surface
    /// Current states are stored int window_state and input_state members
    /// Previous state pointers may be equal to current window_state/input_state to trigger no change
    /// If previous states are nullopt, a full refresh is performed
    void update(
        std::optional<WindowState const*> previous_window_state,
        std::optional<InputState const*> previous_input_state);

    std::shared_ptr<ThreadsafeAccess<BasicDecoration>> const threadsafe_self;
    std::shared_ptr<StaticGeometry const> const static_geometry;

    std::shared_ptr<shell::Shell> const shell;
    std::shared_ptr<graphics::GraphicBufferAllocator> const buffer_allocator;
    std::shared_ptr<input::CursorImages> const cursor_images;
    std::shared_ptr<scene::Session> const session;

    float scale{1.0f};

    class BufferStreams;
    std::unique_ptr<BufferStreams> const buffer_streams;
    std::unique_ptr<Renderer> const renderer;

    std::shared_ptr<scene::Surface> const window_surface;
    std::shared_ptr<scene::Surface> const decoration_surface;
    std::unique_ptr<WindowState const> window_state;

    std::unique_ptr<WindowSurfaceObserverManager> const window_surface_observer_manager;
    std::unique_ptr<InputManager> const input_manager;
    std::unique_ptr<InputState const> input_state;
};
}
}
}

#endif // MIR_SHELL_DECORATION_BASIC_DECORATION_H_
