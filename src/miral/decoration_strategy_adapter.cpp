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

#include <mir/shell/decoration/render_title_text.h>

#include <mir/fatal.h>
#include <mir/graphics/buffer.h>
#include <mir/graphics/graphic_buffer_allocator.h>

#include <memory>
#include <utility>
#include <vector>

namespace miral
{

namespace
{
constexpr auto to_miral_border(mir::shell::decoration::BorderType type) -> DecorationStrategy::BorderType
{
    using Internal = mir::shell::decoration::BorderType;
    switch (type)
    {
    case Internal::Full: return DecorationStrategy::BorderType::full;
    case Internal::Titlebar: return DecorationStrategy::BorderType::titlebar;
    case Internal::None: return DecorationStrategy::BorderType::none;
    }
    std::unreachable();
}

constexpr auto to_miral_button_function(mir::shell::decoration::Button::Function function)
    -> DecorationStrategy::Button::Function
{
    using Internal = mir::shell::decoration::Button::Function;
    switch (function)
    {
    case Internal::Close: return DecorationStrategy::Button::Function::close;
    case Internal::Maximize: return DecorationStrategy::Button::Function::maximize;
    case Internal::Minimize: return DecorationStrategy::Button::Function::minimize;
    }
    std::unreachable();
}

constexpr auto to_miral_button_state(mir::shell::decoration::Button::State state)
    -> DecorationStrategy::Button::State
{
    using Internal = mir::shell::decoration::Button::State;
    switch (state)
    {
    case Internal::Up: return DecorationStrategy::Button::State::up;
    case Internal::Hovered: return DecorationStrategy::Button::State::hovered;
    case Internal::Down: return DecorationStrategy::Button::State::down;
    }
    std::unreachable();
}

auto to_miral(mir::shell::decoration::WindowState const& ws) -> DecorationStrategy::WindowState
{
    return DecorationStrategy::WindowState::build(
        ws.window_id(),
        ws.window_name(),
        ws.focused_state(),
        ws.window_size(),
        to_miral_border(ws.border_type()),
        ws.titlebar_height(),
        ws.side_border_width(),
        ws.bottom_border_height(),
        ws.scale());
}

auto to_miral(mir::shell::decoration::InputState const& input) -> DecorationStrategy::InputState
{
    std::vector<DecorationStrategy::Button> buttons;
    buttons.reserve(input.buttons.size());
    for (auto const& b : input.buttons)
    {
        buttons.push_back({
            .function = to_miral_button_function(b.function),
            .state = to_miral_button_state(b.state),
            .rect = b.rect,
        });
    }
    return DecorationStrategy::InputState::build(std::move(buttons));
}

} // namespace

DecorationStrategyAdapter::DecorationStrategyAdapter(
    std::shared_ptr<miral::DecorationStrategy> user,
    std::shared_ptr<mir::graphics::GraphicBufferAllocator> alloc) :
    user{std::move(user)},
    allocator{std::move(alloc)}
{
    if (!this->user)
        mir::fatal_error("DecorationStrategyAdapter: null user strategy");
    if (!allocator)
        mir::fatal_error("DecorationStrategyAdapter: null allocator");
}

auto DecorationStrategyAdapter::render_strategy() const -> std::unique_ptr<mir::shell::decoration::RendererStrategy>
{
    return std::make_unique<RendererAdapter>(user, allocator);
}

auto DecorationStrategyAdapter::button_placement(unsigned n, mir::shell::decoration::WindowState const& ws) const
    -> mir::geometry::Rectangle
{
    return user->button_placement(n, to_miral(ws));
}

auto DecorationStrategyAdapter::compute_size_with_decorations(
    mir::geometry::Size content_size,
    MirWindowType type,
    MirWindowState state) const -> mir::geometry::Size
{
    return user->compute_size_with_decorations(content_size, type, state);
}

auto DecorationStrategyAdapter::new_window_state(std::shared_ptr<mir::scene::Surface> const& surface, float scale) const
    -> std::unique_ptr<mir::shell::decoration::WindowState>
{
    return std::make_unique<mir::shell::decoration::WindowState>(
        surface, user->titlebar_height(), user->side_border_width(), user->bottom_border_height(), scale);
}

auto DecorationStrategyAdapter::buffer_format() const -> MirPixelFormat { return user->buffer_format(); }

auto DecorationStrategyAdapter::resize_corner_input_size() const -> mir::geometry::Size
{
    return user->resize_corner_input_size();
}

void DecorationStrategy::render_title_text(
    uint32_t* buf,
    mir::geometry::Size buf_size,
    std::string const& text,
    mir::geometry::Point top_left,
    mir::geometry::Height height_pixels,
    uint32_t color)
{
    mir::shell::decoration::render_title_text(
        reinterpret_cast<mir::shell::decoration::Pixel*>(buf), buf_size, text, top_left, height_pixels, color);
}

RendererAdapter::RendererAdapter(
    std::shared_ptr<miral::DecorationStrategy> user,
    std::shared_ptr<mir::graphics::GraphicBufferAllocator> allocator) :
    user{std::move(user)},
    buffers{std::move(allocator)}
{
    if (!this->user)
        mir::fatal_error("RendererAdapter: null user strategy");
}

void RendererAdapter::update_state(
    mir::shell::decoration::WindowState const& ws,
    mir::shell::decoration::InputState const& input)
{
    window_state = to_miral(ws);
    input_state = to_miral(input);
}

auto RendererAdapter::render_from_user(
    std::optional<DecorationSurface> (DecorationStrategy::*render)(
        DecorationStrategy::WindowState const&,
        DecorationStrategy::InputState const&,
        DecorationBuffers&)) -> std::optional<std::shared_ptr<mir::graphics::Buffer>>
{
    if (auto surface = (user.get()->*render)(window_state, input_state, buffers))
        return surface->take_buffer();
    return std::nullopt;
}

auto RendererAdapter::render_titlebar() -> std::optional<std::shared_ptr<mir::graphics::Buffer>>
{
    return render_from_user(&DecorationStrategy::render_titlebar);
}

auto RendererAdapter::render_left_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>>
{
    return render_from_user(&DecorationStrategy::render_left_border);
}

auto RendererAdapter::render_right_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>>
{
    return render_from_user(&DecorationStrategy::render_right_border);
}

auto RendererAdapter::render_bottom_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>>
{
    return render_from_user(&DecorationStrategy::render_bottom_border);
}

}
