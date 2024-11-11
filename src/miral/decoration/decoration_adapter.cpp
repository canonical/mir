/*
 *
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

#include "miral/decoration/decoration_adapter.h"
#include <memory>

namespace md = miral::decoration;

md::DecorationAdapter::DecorationAdapter(
    std::shared_ptr<mir::scene::Surface> decoration_surface, std::shared_ptr<mir::scene::Surface> window_surface) :
    decoration_notifier(decoration_surface, window_surface, this)
{
}

void md::DecorationAdapter::handle_input_event(std::shared_ptr<MirEvent const> const& event)
{
    on_handle_input_event(event);
}

void md::DecorationAdapter::set_scale(float new_scale)
{
    on_set_scale(new_scale);
}

void md::DecorationAdapter::attrib_changed(mir::scene::Surface const* window_surface, MirWindowAttrib attrib, int value)
{
    on_attrib_changed(window_surface, attrib, value);
}

void md::DecorationAdapter::window_resized_to(
    mir::scene::Surface const* window_surface, mir::geometry::Size const& window_size)
{
    on_window_resized_to(window_surface, window_size);
}

void md::DecorationAdapter::window_renamed(mir::scene::Surface const* window_surface, std::string const& name)
{
    on_window_renamed(window_surface, name);
}

md::DecorationBuilder::DecorationBuilder(
    std::shared_ptr<mir::scene::Surface> decoration_surface, std::shared_ptr<mir::scene::Surface> window_surface) :
    adapter{std::unique_ptr<DecorationAdapter>(new DecorationAdapter{decoration_surface, window_surface})}
{
}

auto md::DecorationBuilder::build(
    std::shared_ptr<mir::scene::Surface> decoration_surface,
    std::shared_ptr<mir::scene::Surface> window_surface,
    std::function<void(std::shared_ptr<MirEvent const> const&)> on_handle_input_event,
    std::function<void(float new_scale)> on_set_scale,
    std::function<void(mir::scene::Surface const* window_surface, MirWindowAttrib attrib, int value)> on_attrib_changed,
    std::function<void(mir::scene::Surface const* window_surface, mir::geometry::Size const& window_size)>
        on_window_resized_to,
    std::function<void(mir::scene::Surface const* window_surface, std::string const& name)> on_window_renamed)
    -> DecorationBuilder
{
    auto builder = DecorationBuilder{decoration_surface, window_surface};
    auto& adapter = builder.adapter;

    adapter->on_handle_input_event = on_handle_input_event;
    adapter->on_set_scale = on_set_scale;
    adapter->on_attrib_changed = on_attrib_changed;
    adapter->on_window_resized_to = on_window_resized_to;
    adapter->on_window_renamed = on_window_renamed;

    return builder;
}

auto md::DecorationBuilder::done() -> std::unique_ptr<DecorationAdapter>
{
    return std::move(adapter);
}
