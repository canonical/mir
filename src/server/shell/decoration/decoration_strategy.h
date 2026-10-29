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

#include <mir/geometry/displacement.h>
#include <mir/geometry/point.h>
#include <mir/geometry/rectangle.h>
#include <mir/geometry/size.h>
#include <mir_toolkit/client_types.h>

 #include <endian.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace mir::scene { class Surface; }
namespace mir::graphics { class Buffer; class GraphicBufferAllocator; }

namespace mir::shell::decoration
{
using Pixel = uint32_t;

constexpr auto pack_pixel(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 0xFF) -> Pixel
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return ((Pixel)b <<  0) |
           ((Pixel)g <<  8) |
           ((Pixel)r << 16) |
           ((Pixel)a << 24);
#elif __BYTE_ORDER == __BIG_ENDIAN
    return ((Pixel)b << 24) |
           ((Pixel)g << 16) |
           ((Pixel)r <<  8) |
           ((Pixel)a <<  0);
#else
#error unsupported byte order
#endif
}

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

/// Colors used to draw the titlebar and borders of a window
struct Theme
{
    Pixel background_color;     ///< Color for background of the titlebar and borders
    Pixel text_color;           ///< Color the window title is drawn in
};

/// Colors used to draw a titlebar button
struct ButtonTheme
{
    Pixel normal_color;         ///< Color of the background of the button area
    Pixel active_color;         ///< Color of the background when the button is active
    Pixel icon_color;           ///< Color of the icon
};

/// The appearance of the decorations drawn by the default decoration strategy.
/// Defaults reproduce Mir's built-in look; a shell can override any subset.
struct DecorationStyle
{
    geometry::Height titlebar_height{24};           ///< Visible height of the top titlebar with the window's name and buttons
    geometry::Width side_border_width{6};           ///< Visible width of the side borders
    geometry::Height bottom_border_height{6};       ///< Visible height of the bottom border
    geometry::Size resize_corner_input_size{16, 16};///< The size of the input area of a resizable corner
                                                    ///< (does not effect surface input area, only where in the
                                                    ///< surface is considered a resize corner)
    geometry::Width button_width{24};               ///< The width of window control buttons
    geometry::Width padding_between_buttons{6};     ///< The gap between titlebar buttons
    geometry::Height title_font_height{14};         ///< Height of the text used to render the window title
    geometry::Point title_font_top_left{8, 2};      ///< Where to render the window title
    geometry::Displacement icon_padding{5, 5};      ///< Padding inside buttons around icons
    geometry::Width icon_line_width{1};             ///< Width for lines in button icons

    Theme focused{pack_pixel(0x32, 0x32, 0x32), pack_pixel(0xFF, 0xFF, 0xFF)};      ///< Colors used while focused
    Theme unfocused{pack_pixel(0x80, 0x80, 0x80), pack_pixel(0xA0, 0xA0, 0xA0)};    ///< Colors used while unfocused

    ButtonTheme button{
        pack_pixel(0x60, 0x60, 0x60),
        pack_pixel(0xA0, 0xA0, 0xA0),
        pack_pixel(0xFF, 0xFF, 0xFF)};              ///< Colors used for the minimize and maximize buttons
    ButtonTheme close_button{
        pack_pixel(0xA0, 0x20, 0x20),
        pack_pixel(0xC0, 0x60, 0x60),
        pack_pixel(0xFF, 0xFF, 0xFF)};              ///< Colors used for the close button

    MirPixelFormat buffer_format{mir_pixel_format_argb_8888};
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

    static auto default_decoration_strategy(
        std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<DecorationStyle const> const& style = std::make_shared<DecorationStyle const>())
        -> std::shared_ptr<DecorationStrategy>;

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
