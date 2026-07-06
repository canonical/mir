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

#include <miral/decoration_strategy.h>

#include <utility>

namespace miral
{

struct DecorationStrategy::InputState::Self
{
    std::vector<Button> buttons;
};

struct DecorationStrategy::WindowState::Self
{
    uint64_t window_id{};
    std::string window_name;
    MirWindowFocusState focused_state{};
    mir::geometry::Size window_size{};
    BorderType border_type{BorderType::none};
    mir::geometry::Height titlebar_height{};
    mir::geometry::Width side_border_width{};
    mir::geometry::Height bottom_border_height{};
    float scale{1.0f};
};

DecorationStrategy::InputState::InputState() : self{std::make_unique<Self>()} {}

DecorationStrategy::InputState::InputState(InputState const& other) :
    self{std::make_unique<Self>(*other.self)}
{}

auto DecorationStrategy::InputState::operator=(InputState const& other) -> InputState&
{
    if (this != &other)
        *self = *other.self;
    return *this;
}

DecorationStrategy::InputState::InputState(InputState&&) noexcept = default;

auto DecorationStrategy::InputState::operator=(InputState&&) noexcept -> InputState& = default;

DecorationStrategy::InputState::~InputState() = default;

auto DecorationStrategy::InputState::build(std::vector<Button> buttons) -> InputState
{
    InputState input;
    input.self->buttons = std::move(buttons);
    return input;
}

auto DecorationStrategy::InputState::buttons() const -> std::vector<Button> const&
{
    return self->buttons;
}

DecorationStrategy::WindowState::WindowState() : self{std::make_unique<Self>()} {}

DecorationStrategy::WindowState::WindowState(WindowState const& other) :
    self{std::make_unique<Self>(*other.self)}
{}

auto DecorationStrategy::WindowState::operator=(WindowState const& other) -> WindowState&
{
    if (this != &other)
        *self = *other.self;
    return *this;
}

DecorationStrategy::WindowState::WindowState(WindowState&&) noexcept = default;

auto DecorationStrategy::WindowState::operator=(WindowState&&) noexcept -> WindowState& = default;

DecorationStrategy::WindowState::~WindowState() = default;

auto DecorationStrategy::WindowState::build(
    uint64_t window_id,
    std::string window_name,
    MirWindowFocusState focused_state,
    mir::geometry::Size window_size,
    BorderType border_type,
    mir::geometry::Height titlebar_height,
    mir::geometry::Width side_border_width,
    mir::geometry::Height bottom_border_height,
    float scale) -> WindowState
{
    WindowState ws;
    ws.self->window_id = window_id;
    ws.self->window_name = std::move(window_name);
    ws.self->focused_state = focused_state;
    ws.self->window_size = window_size;
    ws.self->border_type = border_type;
    ws.self->titlebar_height = titlebar_height;
    ws.self->side_border_width = side_border_width;
    ws.self->bottom_border_height = bottom_border_height;
    ws.self->scale = scale;
    return ws;
}

auto DecorationStrategy::WindowState::window_id() const -> uint64_t { return self->window_id; }

auto DecorationStrategy::WindowState::window_name() const -> std::string const& { return self->window_name; }

auto DecorationStrategy::WindowState::focused_state() const -> MirWindowFocusState { return self->focused_state; }

auto DecorationStrategy::WindowState::window_size() const -> mir::geometry::Size { return self->window_size; }

auto DecorationStrategy::WindowState::border_type() const -> BorderType { return self->border_type; }

auto DecorationStrategy::WindowState::titlebar_height() const -> mir::geometry::Height { return self->titlebar_height; }

auto DecorationStrategy::WindowState::side_border_width() const -> mir::geometry::Width { return self->side_border_width; }

auto DecorationStrategy::WindowState::bottom_border_height() const -> mir::geometry::Height
{
    return self->bottom_border_height;
}

auto DecorationStrategy::WindowState::scale() const -> float { return self->scale; }

auto DecorationStrategy::WindowState::titlebar_width() const -> mir::geometry::Width { return self->window_size.width; }

}