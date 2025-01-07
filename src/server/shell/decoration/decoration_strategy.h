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

namespace mir::shell::decoration
{
using Pixel = uint32_t;

enum class ButtonState
{
    Up,         ///< The user is not interacting with this button
    Hovered,    ///< The user is hovering over this button
    Down,       ///< The user is currently pressing this button
};

enum class ButtonFunction
{
    Close,
    Maximize,
    Minimize,
};

enum class BorderType
{
    Full,       ///< Full titlebar and border (for restored windows)
    Titlebar,   ///< Titlebar only (for maximized windows)
    None,       ///< No decorations (for fullscreen windows or popups)
};

auto border_type_for(MirWindowType type, MirWindowState state) -> BorderType;

struct ButtonInfo
{
    ButtonFunction function;
    ButtonState state;
    geometry::Rectangle rect;

    auto operator==(ButtonInfo const& other) const -> bool
    {
        return function == other.function &&
               state == other.state &&
               rect == other.rect;
    }
};

/// Describes the state of the interface (what buttons are pushed, etc)
class InputState
{
public:
    InputState(
        std::vector<ButtonInfo> const& buttons,
        std::vector<geometry::Rectangle> const& input_shape)
        : buttons_{buttons},
          input_shape_{input_shape}
    {
    }

    auto buttons() const -> std::vector<ButtonInfo> const& { return buttons_; }
    auto input_shape() const -> std::vector<geometry::Rectangle> const& { return input_shape_; }

private:
    InputState(InputState const&) = delete;
    InputState& operator=(InputState const&) = delete;

    std::vector<ButtonInfo> const buttons_;
    std::vector<geometry::Rectangle> const input_shape_;
};

class DecorationStrategy;

/// Information about the geometry and type of decorations for a given window
/// Data is pulled from the surface on construction and immutable after that
class WindowState
{
public:
    WindowState(
        std::shared_ptr<const DecorationStrategy>&& decoration_strategy,
        std::shared_ptr<scene::Surface> const& window_surface,
        float scale);

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
    WindowState& operator=(WindowState const&) = delete;

    std::shared_ptr<const DecorationStrategy> const decoration_strategy;
    geometry::Size const window_size_;
    BorderType const border_type_;
    MirWindowFocusState const focus_state_;
    std::string const window_name_;
    float const scale_;
};

class RendererStrategy
{
public:
    static auto alloc_pixels(geometry::Size size) -> std::unique_ptr<Pixel[]>;

    RendererStrategy() = default;
    virtual ~RendererStrategy() = default;

    struct RenderedPixels
    {
        MirPixelFormat const format;
        geometry::Size const size;
        Pixel const* const pixels;
    };

    virtual void update_state(WindowState const& window_state, InputState const& input_state) = 0;
    virtual auto render_titlebar() -> std::optional<RenderedPixels> = 0;
    virtual auto render_left_border() -> std::optional<RenderedPixels> = 0;
    virtual auto render_right_border() -> std::optional<RenderedPixels> = 0;
    virtual auto render_bottom_border() -> std::optional<RenderedPixels> = 0;
};

class DecorationStrategy
{
public:

    static auto default_decoration_strategy() -> std::shared_ptr<DecorationStrategy>;

    virtual auto render_strategy() const -> std::unique_ptr<RendererStrategy> = 0;
    virtual auto button_placement(unsigned n, WindowState const& ws) const -> geometry::Rectangle = 0;
    virtual auto compute_size_with_decorations(
        geometry::Size content_size, MirWindowType type, MirWindowState state) const -> geometry::Size = 0;
    virtual auto new_window_state(
        std::shared_ptr<scene::Surface> const& window_surface, float scale) const -> std::unique_ptr<WindowState> = 0;
    virtual auto buffer_format() const -> MirPixelFormat = 0;
    virtual auto resize_corner_input_size() const -> geometry::Size = 0;
    virtual auto titlebar_height() const -> geometry::Height = 0;
    virtual auto side_border_width() const -> geometry::Width = 0;
    virtual auto bottom_border_height() const -> geometry::Height = 0;

    DecorationStrategy() = default;
    virtual ~DecorationStrategy() = default;

private:
    DecorationStrategy(DecorationStrategy const&) = delete;
    DecorationStrategy& operator=(DecorationStrategy const&) = delete;
};
}

#endif //MIR_SHELL_DECORATION_STRATEGY_H_
