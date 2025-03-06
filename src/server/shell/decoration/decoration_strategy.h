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

#ifndef MIR_SHELL_DECORATION_STRATEGY_H_
#define MIR_SHELL_DECORATION_STRATEGY_H_

#include "mir/geometry/displacement.h"
#include "mir/geometry/point.h"
#include "mir/geometry/rectangle.h"
#include "mir/geometry/size.h"
#include "mir_toolkit/client_types.h"

#include <memory>
#include <optional>
#include <vector>

namespace mir::scene { class Surface; }
namespace mir::graphics { class Buffer; class GraphicBufferAllocator; }

namespace mir::shell::decoration
{
enum class BorderType
{
    Full,       ///< Full titlebar and border (for restored windows)
    Titlebar,   ///< Titlebar only (for maximized windows)
    None,       ///< No decorations (for fullscreen windows or popups)
};

auto border_type_for(MirWindowType type, MirWindowState state) -> BorderType;

/// Minimize, maximize, and close buttons
struct Button
{
    enum class Function
    {
        Close,
        Maximize,
        Minimize,
    };
    using enum Function;

    enum class State
    {
        Up,         ///< The user is not interacting with this button
        Hovered,    ///< The user is hovering over this button
        Down,       ///< The user is currently pressing this button
    };
    using enum State;

    Function function;
    State state;
    geometry::Rectangle rect;

    auto operator==(Button const& other) const -> bool = default;
};

/// Describes the state of the interface (what buttons are pushed, etc)
struct InputState
{
    std::vector<Button> const buttons;
    std::vector<geometry::Rectangle> const input_shape;
};

/// Information about the geometry and type of decorations for a given window
/// Data is pulled from the surface on construction and immutable after that
class WindowState
{
public:
    WindowState(
        std::shared_ptr<scene::Surface> const& window_surface,
        geometry::Height fixed_titlebar_height,
        geometry::Width fixed_side_border_width,
        geometry::Height fixed_bottom_border_height,
        float scale);

    ~WindowState();

    auto window_size() const -> geometry::Size;
    auto border_type() const -> BorderType;
    auto focused_state() const -> MirWindowFocusState;
    auto window_name() const -> std::string;

    auto titlebar_width() const -> geometry::Width;
    auto titlebar_height() const -> geometry::Height;

    auto side_border_width() const -> geometry::Width;
    auto side_border_height() const -> geometry::Height;

    auto bottom_border_width() const -> geometry::Width;
    auto bottom_border_height() const -> geometry::Height;

    auto titlebar_rect() const -> geometry::Rectangle;
    auto left_border_rect() const -> geometry::Rectangle;
    auto right_border_rect() const -> geometry::Rectangle;
    auto bottom_border_rect() const -> geometry::Rectangle;

    auto scale() const -> float;

private:
    WindowState(WindowState const&) = delete;

    class Self;
    std::unique_ptr<Self> const self;
};


using Pixel = uint32_t;

/// Customization point for rendering
class RendererStrategy
{
public:
    RendererStrategy() = default;
    virtual ~RendererStrategy() = default;

    virtual void update_state(WindowState const& window_state, InputState const& input_state) = 0;
    virtual auto render_titlebar() -> std::optional<std::shared_ptr<graphics::Buffer>> = 0;
    virtual auto render_left_border() -> std::optional<std::shared_ptr<graphics::Buffer>> = 0;
    virtual auto render_right_border() -> std::optional<std::shared_ptr<graphics::Buffer>> = 0;
    virtual auto render_bottom_border() -> std::optional<std::shared_ptr<graphics::Buffer>> = 0;
};

/// Customization point for decorations
class DecorationStrategy
{
public:

    static auto default_decoration_strategy(std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& allocator) -> std::shared_ptr<DecorationStrategy>;

    virtual auto render_strategy() const -> std::unique_ptr<RendererStrategy> = 0;
    virtual auto button_placement(unsigned n, WindowState const& ws) const -> geometry::Rectangle = 0;
    virtual auto compute_size_with_decorations(
        geometry::Size content_size, MirWindowType type, MirWindowState state) const -> geometry::Size = 0;
    virtual auto new_window_state(
        std::shared_ptr<scene::Surface> const& window_surface, float scale) const -> std::unique_ptr<WindowState> = 0;
    virtual auto buffer_format() const -> MirPixelFormat = 0;
    virtual auto resize_corner_input_size() const -> geometry::Size = 0;

    DecorationStrategy() = default;
    virtual ~DecorationStrategy() = default;

private:
    DecorationStrategy(DecorationStrategy const&) = delete;
    DecorationStrategy& operator=(DecorationStrategy const&) = delete;
};
}

#endif //MIR_SHELL_DECORATION_STRATEGY_H_
