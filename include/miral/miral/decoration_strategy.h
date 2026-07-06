/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_DECORATION_STRATEGY_H_
#define MIRAL_DECORATION_STRATEGY_H_

#include <mir/geometry/rectangle.h>
#include <mir/geometry/size.h>
#include <miral/decoration_buffers.h>
#include <miral/decoration_surface.h>
#include <mir_toolkit/common.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace miral
{

/// Pack a color suitable for writing via DecorationBuffers::Mapping::pixels32() or
/// passing to DecorationStrategy::render_title_text().
///
/// On any host, `pixels32()[i] = pack_color(a, r, g, b);` will result in the
/// correct memory byte layout for mir_pixel_format_argb_8888 when used with Mir.
constexpr uint32_t pack_color(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return (uint32_t(blue) << 0) | (uint32_t(green) << 8) | (uint32_t(red) << 16) | (uint32_t(alpha) << 24);
#elif __BYTE_ORDER == __BIG_ENDIAN
    return (uint32_t(blue) << 24) | (uint32_t(green) << 16) | (uint32_t(red) << 8) | (uint32_t(alpha) << 0);
#else
  #error "MirAL decorations require a known byte order"
#endif
}

/// Limited buffer-based customization of server-side decorations.
/// Implementations must be stateless: all context is passed into each render call.
/// \remark Since MirAL 6.0
class DecorationStrategy
{
public:
    enum class BorderType
    {
        full,     ///< Full titlebar and border (for restored windows)
        titlebar, ///< Titlebar only (for maximized windows)
        none,     ///< No decorations (for fullscreen windows or popups)
    };

    struct Button
    {
        enum class Function
        {
            close,
            maximize,
            minimize,
        };

        enum class State
        {
            up,
            hovered,
            down,
        };

        Function function;
        State state;
        mir::geometry::Rectangle rect;
    };

    /// Per-frame input state passed into each render call.
    /// Only the standard close/maximize/minimize buttons are exposed in v1;
    /// internal input geometry (e.g. non-button hit regions) is not available.
    /// \remark Since MirAL 6.0
    class InputState
    {
    public:
        InputState();
        InputState(InputState const&);
        auto operator=(InputState const&) -> InputState&;
        InputState(InputState&&) noexcept;
        auto operator=(InputState&&) noexcept -> InputState&;
        ~InputState();

        static auto build(std::vector<Button> buttons) -> InputState;

        auto buttons() const -> std::vector<Button> const&;

    private:
        struct Self;
        std::unique_ptr<Self> self;
    };

    /// Per-window immutable state for a single render call.
    /// window_id() is an opaque token stable for the lifetime of the decorated window;
    /// compositors may use it to key per-window render caches. It is not a
    /// miral::Window handle and is not preserved if the underlying surface is destroyed
    /// and recreated.
    /// \remark Since MirAL 6.0
    class WindowState
    {
    public:
        WindowState();
        WindowState(WindowState const&);
        auto operator=(WindowState const&) -> WindowState&;
        WindowState(WindowState&&) noexcept;
        auto operator=(WindowState&&) noexcept -> WindowState&;
        ~WindowState();

        static auto build(
            uint64_t window_id,
            std::string window_name,
            MirWindowFocusState focused_state,
            mir::geometry::Size window_size,
            BorderType border_type,
            mir::geometry::Height titlebar_height,
            mir::geometry::Width side_border_width,
            mir::geometry::Height bottom_border_height,
            float scale) -> WindowState;

        auto window_id() const -> uint64_t;
        auto window_name() const -> std::string const&;
        auto focused_state() const -> MirWindowFocusState;
        auto window_size() const -> mir::geometry::Size;
        auto border_type() const -> BorderType;
        auto titlebar_height() const -> mir::geometry::Height;
        auto side_border_width() const -> mir::geometry::Width;
        auto bottom_border_height() const -> mir::geometry::Height;
        auto scale() const -> float;
        auto titlebar_width() const -> mir::geometry::Width;

    private:
        struct Self;
        std::unique_ptr<Self> self;
    };

    virtual ~DecorationStrategy() = default;

    virtual auto compute_size_with_decorations(
        mir::geometry::Size content_size,
        MirWindowType type,
        MirWindowState state) const -> mir::geometry::Size = 0;

    virtual auto button_placement(unsigned n, WindowState const& ws) const -> mir::geometry::Rectangle = 0;

    virtual auto render_titlebar(WindowState const& ws, InputState const& input, DecorationBuffers& buffers)
        -> std::optional<DecorationSurface> = 0;

    virtual auto render_left_border(WindowState const& ws, InputState const& input, DecorationBuffers& buffers)
        -> std::optional<DecorationSurface> = 0;

    virtual auto render_right_border(WindowState const& ws, InputState const& input, DecorationBuffers& buffers)
        -> std::optional<DecorationSurface> = 0;

    virtual auto render_bottom_border(WindowState const& ws, InputState const& input, DecorationBuffers& buffers)
        -> std::optional<DecorationSurface> = 0;

    virtual auto buffer_format() const -> MirPixelFormat = 0;
    virtual auto resize_corner_input_size() const -> mir::geometry::Size = 0;

    // Controls internal decoration geometry (distinct from compute_size_with_decorations()).
    virtual auto titlebar_height() const -> mir::geometry::Height { return mir::geometry::Height{24}; }
    virtual auto side_border_width() const -> mir::geometry::Width { return mir::geometry::Width{4}; }
    virtual auto bottom_border_height() const -> mir::geometry::Height { return mir::geometry::Height{4}; }

    virtual void render_title_text(
        uint32_t* buf,
        mir::geometry::Size buf_size,
        std::string const& text,
        mir::geometry::Point top_left,
        mir::geometry::Height height_pixels,
        uint32_t color);
};

}

#endif // MIRAL_DECORATION_STRATEGY_H_