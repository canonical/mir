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

#include "decoration_strategy_adapter.h"

#include <mir/fatal.h>
#include <mir/geometry/rectangle.h>
#include <mir/graphics/graphic_buffer_allocator.h>
#include <mir/renderer/sw/pixel_source.h>
#include <mir/scene/surface.h>
#include <mir/shell/decoration/decoration_strategy.h>

#include <cstddef>
#include <filesystem>
#include <memory>

namespace miral
{

namespace
{
static auto to_miral(::mir::shell::decoration::WindowState const& ws) -> WindowState
{
    WindowState m;
    m.window_name = ws.window_name();
    m.window_size = ws.window_size();
    m.border_type = static_cast<BorderType>(static_cast<int>(ws.border_type()));
    m.titlebar_height_ = ws.titlebar_height();
    m.side_border_width = ws.side_border_width();
    m.bottom_border_height = ws.bottom_border_height();
    m.scale = ws.scale();
    m.focused_state = static_cast<MirWindowFocusState>(static_cast<int>(ws.focused_state()));
    return m;
}

static auto to_miral(::mir::shell::decoration::InputState const& input) -> InputState
{
    InputState m;
    for (auto const& b : input.buttons)
    {
        Button mb;
        mb.function = static_cast<Button::Function>(static_cast<int>(b.function));
        mb.state = static_cast<Button::State>(static_cast<int>(b.state));
        mb.rect = b.rect;
        m.buttons.push_back(mb);
    }
    return m;
}

} // namespace

DecorationStrategyAdapter::DecorationStrategyAdapter(
    std::shared_ptr<miral::DecorationStrategy> user,
    std::shared_ptr<mir::graphics::GraphicBufferAllocator> alloc) :
    user(std::move(user)),
    allocator(alloc) // strong (adapter); base weak only
{
    if (!this->user)
        mir::fatal_error("DecorationStrategyAdapter: null user strategy in ctor");
    this->user->set_buffer_allocator(alloc); // pass shared
}

auto DecorationStrategyAdapter::render_strategy() const -> std::unique_ptr<mir::shell::decoration::RendererStrategy>
{
    return std::make_unique<RendererAdapter>(user);
}

auto DecorationStrategyAdapter::button_placement(unsigned n, mir::shell::decoration::WindowState const& ws) const
    -> mir::geometry::Rectangle
{
    auto mws = to_miral(ws);
    return user->button_placement(n, mws);
}

auto DecorationStrategyAdapter::compute_size_with_decorations(
    mir::geometry::Size content_size,
    MirWindowType type,
    MirWindowState state) const -> mir::geometry::Size
{
    return user->compute_size_with_decorations(
        content_size,
        static_cast<MirWindowType>(static_cast<int>(type)),
        static_cast<MirWindowState>(static_cast<int>(state)));
}

auto DecorationStrategyAdapter::new_window_state(std::shared_ptr<mir::scene::Surface> const& surface, float scale) const
    -> std::unique_ptr<mir::shell::decoration::WindowState>
{
    // Values come from the user DecorationStrategy (via overridable titlebar_height()/
    // side_border_width()/bottom_border_height(); defaults 24/4/4).
    // These drive internal WindowState geometry/input for BasicDecoration.
    // (User's compute_size_with_decorations() only affects client-reported size.)
    return std::make_unique<mir::shell::decoration::WindowState>(
        surface, user->titlebar_height(), user->side_border_width(), user->bottom_border_height(), scale);
}

auto DecorationStrategyAdapter::buffer_format() const -> ::MirPixelFormat
{
    return static_cast<MirPixelFormat>(static_cast<int>(user->buffer_format()));
}

auto DecorationStrategyAdapter::resize_corner_input_size() const -> mir::geometry::Size
{
    return user->resize_corner_input_size();
}

// Default implementation for miral::DecorationStrategy::render_title_text() (delegates
// to internal FT for title text when not overridden by user).
void DecorationStrategy::render_title_text(
    uint32_t* buf,
    mir::geometry::Size buf_size,
    std::string const& text,
    mir::geometry::Point top_left,
    mir::geometry::Height height_pixels,
    uint32_t color)
{
    ::mir::shell::decoration::render_title_text(
        reinterpret_cast<::mir::shell::decoration::Pixel*>(buf), buf_size, text, top_left, height_pixels, color);
}

RendererAdapter::RendererAdapter(std::shared_ptr<miral::DecorationStrategy> user) : user(std::move(user))
{
    if (!this->user)
        mir::fatal_error("RendererAdapter: null user strategy in ctor");
}

void RendererAdapter::update_state(
    mir::shell::decoration::WindowState const& ws,
    mir::shell::decoration::InputState const& input)
{
    auto mws = to_miral(ws);
    auto minput = to_miral(input);
    user->update_state(mws, minput);
}

auto RendererAdapter::render_titlebar() -> std::optional<std::shared_ptr<mir::graphics::Buffer>>
{
    return user->render_titlebar();
}

auto RendererAdapter::render_left_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>>
{
    return user->render_left_border();
}

auto RendererAdapter::render_right_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>>
{
    return user->render_right_border();
}

auto RendererAdapter::render_bottom_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>>
{
    return user->render_bottom_border();
}

void DecorationStrategy::set_buffer_allocator(std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& alloc)
{
    allocator = alloc; // weak in base (adapter holds strong)
}

auto DecorationStrategy::create_software_buffer(mir::geometry::Size size) -> std::shared_ptr<mir::graphics::Buffer>
{
    auto a = allocator.lock();
    if (!a)
        mir::fatal_error("DecorationStrategy: no allocator set (or expired at shutdown)");
    return a->alloc_software_buffer(size, mir_pixel_format_argb_8888);
}

struct SoftwareBufferMapping::Self
{
    Self(std::shared_ptr<mir::graphics::Buffer> const& buffer) :
        mappable{mir::renderer::software::as_write_mappable(buffer)},
        mapping{mappable->map_writeable()}
    {}

    std::shared_ptr<mir::renderer::software::WriteMappable> mappable;
    std::unique_ptr<mir::renderer::software::Mapping<std::byte>> mapping;
};

SoftwareBufferMapping::SoftwareBufferMapping(std::shared_ptr<mir::graphics::Buffer> buffer) :
    self{std::make_unique<Self>(buffer)}
{}

SoftwareBufferMapping::~SoftwareBufferMapping() = default;

auto SoftwareBufferMapping::data() -> uint8_t* { return reinterpret_cast<uint8_t*>(self->mapping->data()); }

auto SoftwareBufferMapping::stride() const -> mir::geometry::Stride { return self->mapping->stride(); }

auto SoftwareBufferMapping::size() const -> mir::geometry::Size { return self->mapping->size(); }

auto SoftwareBufferMapping::format() const -> MirPixelFormat { return self->mapping->format(); }

auto SoftwareBufferMapping::pixels32() -> uint32_t*
{
    // Assumes the buffer is argb_8888 (as created by create_software_buffer)
    return reinterpret_cast<uint32_t*>(self->mapping->data());
}

auto DecorationStrategy::map_software_buffer(std::shared_ptr<mir::graphics::Buffer> const& buffer)
    -> SoftwareBufferMapping
{
    return SoftwareBufferMapping{buffer};
}

}
