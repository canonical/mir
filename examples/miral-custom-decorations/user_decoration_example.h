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

#ifndef MIRAL_CUSTOM_DECORATIONS_USER_DECORATION_EXAMPLE_H
#define MIRAL_CUSTOM_DECORATIONS_USER_DECORATION_EXAMPLE_H

#include "renderer.h"
#include "theme.h"
#include "window.h"

#include <memory>

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
}
}

class UserDecorationExample
{
public:
    UserDecorationExample(
        std::shared_ptr<mir::shell::Shell> const& shell,
        std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<mir::Executor> const& executor,
        std::shared_ptr<mir::input::CursorImages> const& cursor_images,
        std::shared_ptr<mir::scene::Surface> const& window_surface);

    UserDecorationExample(
        std::shared_ptr<mir::shell::Shell> const& shell,
        std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<mir::Executor> const& executor,
        std::shared_ptr<mir::input::CursorImages> const& cursor_images,
        std::shared_ptr<mir::scene::Surface> const& window_surface,
        Theme focused,
        Theme unfocused,
        ButtonTheme button_theme);

    ~UserDecorationExample();

    void handle_input_event(std::shared_ptr<MirEvent const> const& event);
    void set_scale(float scale);
    void attrib_changed(mir::scene::Surface const* window_surface, MirWindowAttrib attrib, int value);
    void window_resized_to(mir::scene::Surface const* window_surface, mir::geometry::Size const& window_size);
    void window_renamed(mir::scene::Surface const* window_surface, std::string const& name);

    void input_state_changed();
    void request_move(MirInputEvent const* event);
    void request_resize(MirInputEvent const* event, MirResizeEdge edge);
    void request_toggle_maximize();
    void request_minimize();
    void request_close();

    void set_cursor(std::string const& cursor_image_name);

    std::shared_ptr<mir::scene::Surface> decoration_surface();
    std::shared_ptr<mir::scene::Surface> window_surface();

private:

    void redraw();

    /// Creates an up-to-date WindowState object
    auto new_window_state() const -> std::unique_ptr<WindowState>;

    /// Draw the decoration buffers and submit them to the surface
    /// Current states are stored int window_state and input_state members
    /// Previous state pointers may be equal to current window_state/input_state to trigger no change
    /// If previous states are nullopt, a full refresh is performed
    void update(
        std::optional<WindowState const*> previous_window_state,
        std::optional<InputState const*> previous_input_state);

    std::shared_ptr<mir::scene::Surface> decoration_surface_;

    std::shared_ptr<ThreadsafeAccess<UserDecorationExample>> const threadsafe_self;
    std::shared_ptr<StaticGeometry const> const static_geometry;

    std::shared_ptr<mir::shell::Shell> const shell;
    std::shared_ptr<mir::graphics::GraphicBufferAllocator> const buffer_allocator;
    std::shared_ptr<mir::input::CursorImages> const cursor_images;
    std::shared_ptr<mir::scene::Session> const session;

    float scale{1.0f};

    class BufferStreams;
    std::unique_ptr<BufferStreams> const buffer_streams;
    std::unique_ptr<Renderer> const renderer;

    std::shared_ptr<mir::scene::Surface> const window_surface_;
    std::unique_ptr<WindowState const> window_state;

    std::unique_ptr<InputManager> const input_manager;
    std::unique_ptr<InputState const> input_state;
};

#endif
