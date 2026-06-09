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
#include <mir_toolkit/common.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mir {
namespace graphics {
class Buffer;
class GraphicBufferAllocator;
}
}

namespace mir { class Server; }

namespace miral
{

/// Pack a color suitable for writing via SoftwareBufferMapping::pixels32() or
/// passing to DecorationStrategy::render_title_text().
///
/// On any host, `pixels32()[i] = pack_color(a, r, g, b);` (or the equivalent
/// literal constructed the same way) will result in the correct memory byte
/// layout for mir_pixel_format_argb_8888 when the buffer is used with Mir
/// (consistent with the internal pack_pixel convention and Wayland's little-
/// endian format definitions).
constexpr uint32_t pack_color(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return (uint32_t(blue)  <<  0) |
           (uint32_t(green) <<  8) |
           (uint32_t(red)   << 16) |
           (uint32_t(alpha) << 24);
#elif __BYTE_ORDER == __BIG_ENDIAN
    return (uint32_t(blue)  << 24) |
           (uint32_t(green) << 16) |
           (uint32_t(red)   <<  8) |
           (uint32_t(alpha) <<  0);
#else
#   error "MirAL decorations require a known byte order"
#endif
}

enum class BorderType
{
    full,       ///< Full titlebar and border (for restored windows)
    titlebar,   ///< Titlebar only (for maximized windows)
    none,       ///< No decorations (for fullscreen windows or popups)
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
        up,         ///< The user is not interacting with this button
        hovered,    ///< The user is hovering over this button
        down,       ///< The user is currently pressing this button
    };

    Function function;
    State state;
    mir::geometry::Rectangle rect;
};

struct InputState
{
    std::vector<Button> buttons;
};

struct WindowState
{
    std::string window_name;
    MirWindowFocusState focused_state;
    mir::geometry::Size window_size;
    BorderType border_type;
    mir::geometry::Height titlebar_height_;  // supports titlebar_height() accessor
    mir::geometry::Width side_border_width;
    mir::geometry::Height bottom_border_height;
    float scale;

    mir::geometry::Width titlebar_width() const { return window_size.width; }
    mir::geometry::Height titlebar_height() const { return titlebar_height_; }
};

class SoftwareBufferMapping
{
public:
    explicit SoftwareBufferMapping(std::shared_ptr<mir::graphics::Buffer> buffer);
    ~SoftwareBufferMapping();

    auto data() -> uint8_t*;
    auto stride() const -> mir::geometry::Stride;
    auto size() const -> mir::geometry::Size;
    auto format() const -> MirPixelFormat;
    auto pixels32() -> uint32_t*;

private:
    struct Self;
    std::unique_ptr<Self> self;
};

class DecorationStrategy
{
public:
    virtual ~DecorationStrategy() = default;

    virtual auto compute_size_with_decorations(
        mir::geometry::Size content_size, MirWindowType type, MirWindowState state) const
        -> mir::geometry::Size = 0;
    virtual auto button_placement(unsigned n, WindowState const& ws) const -> mir::geometry::Rectangle = 0;
    virtual void update_state(WindowState const& ws, InputState const& input) = 0;

    virtual auto render_titlebar() -> std::optional<std::shared_ptr<mir::graphics::Buffer>> = 0;
    virtual auto render_left_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>> = 0;
    virtual auto render_right_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>> = 0;
    virtual auto render_bottom_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>> = 0;

    virtual auto buffer_format() const -> MirPixelFormat = 0;
    virtual auto resize_corner_input_size() const -> mir::geometry::Size = 0;

    // Overriding titlebar_height()/side_border_width()/bottom_border_height() controls
    // *internal* decoration geometry used for rendering and input (distinct from the
    // client-visible size returned by compute_size_with_decorations()).
    virtual auto titlebar_height() const -> mir::geometry::Height { return mir::geometry::Height{24}; }
    virtual auto side_border_width() const -> mir::geometry::Width  { return mir::geometry::Width{4}; }
    virtual auto bottom_border_height() const -> mir::geometry::Height { return mir::geometry::Height{4}; }

    // Has a default implementation (delegates to Mir’s existing FreeType renderer)
    // while the other rendering methods are pure virtual.
    virtual void render_title_text(
        uint32_t* buf,
        mir::geometry::Size buf_size,
        std::string const& text,
        mir::geometry::Point top_left,
        mir::geometry::Height height_pixels,
        uint32_t color);
    virtual void set_buffer_allocator(std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& allocator);

protected:
    auto create_software_buffer(mir::geometry::Size size)
        -> std::shared_ptr<mir::graphics::Buffer>;

    auto map_software_buffer(std::shared_ptr<mir::graphics::Buffer> const& buffer)
        -> SoftwareBufferMapping;

private:
    std::weak_ptr<mir::graphics::GraphicBufferAllocator> allocator;
};

}

#endif  // MIRAL_DECORATION_STRATEGY_H_
