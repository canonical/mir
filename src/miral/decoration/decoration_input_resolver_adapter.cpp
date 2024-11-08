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

#ifndef MIRAL_DECORATION_DECORATION_INPUT_RESOLVER_ADAPTER_H
#define MIRAL_DECORATION_DECORATION_INPUT_RESOLVER_ADAPTER_H

#include "miral/decoration/decoration_input_resolver_adapter.h"

namespace md = miral::decoration;

md::InputResolverAdapter::InputResolverAdapter() = default;

void md::InputResolverAdapter::process_enter(DeviceEvent& device)
{
    on_process_enter(device);
}

void md::InputResolverAdapter::process_leave()
{
    on_process_leave();
}

void md::InputResolverAdapter::process_down()
{
    on_process_down();
}

void md::InputResolverAdapter::process_up()
{
    on_process_up();
}

void md::InputResolverAdapter::process_move(DeviceEvent& device)
{
    on_process_move(device);
}

void md::InputResolverAdapter::process_drag(DeviceEvent& device)
{
    on_process_drag(device);
}

md::InputResolverBuilder::InputResolverBuilder() :
    adapter{std::unique_ptr<InputResolverAdapter>(new InputResolverAdapter())}
{
}

auto md::InputResolverBuilder::build(
    std::function<void(DeviceEvent& device)> on_process_enter,
    std::function<void()> on_process_leave,
    std::function<void()> on_process_down,
    std::function<void()> on_process_up,
    std::function<void(DeviceEvent& device)> on_process_move,
    std::function<void(DeviceEvent& device)> on_process_drag) -> InputResolverBuilder
{
    auto builder = InputResolverBuilder{};
    auto& adapter = builder.adapter;

    adapter->on_process_enter = on_process_enter;
    adapter->on_process_leave = on_process_leave;
    adapter->on_process_down = on_process_down;
    adapter->on_process_up = on_process_up;
    adapter->on_process_move = on_process_move;
    adapter->on_process_drag = on_process_drag;

    return builder;
}

auto md::InputResolverBuilder::done() -> std::unique_ptr<InputResolverAdapter>
{
    return std::move(adapter);
}

#endif
