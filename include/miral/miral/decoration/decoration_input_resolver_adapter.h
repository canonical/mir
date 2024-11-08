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

#ifndef MIRAL_DECORATION_DECORATION_INPUT_MANAGER_ADAPTER_H
#define MIRAL_DECORATION_DECORATION_INPUT_MANAGER_ADAPTER_H

#include "mir/shell/input_resolver.h"

#include <functional>
#include <memory>

namespace miral::decoration
{
class InputResolverBuilder;
class InputResolverAdapter : public mir::shell::decoration::InputResolver
{
protected:
    void process_enter(DeviceEvent& device) override;
    void process_leave() override;
    void process_down() override;
    void process_up() override;
    void process_move(DeviceEvent& device) override;
    void process_drag(DeviceEvent& device) override;

private:
    friend InputResolverBuilder;
    InputResolverAdapter();

    std::function<void(DeviceEvent& device)> on_process_enter;
    std::function<void()> on_process_leave;
    std::function<void()> on_process_down;
    std::function<void()> on_process_up;
    std::function<void(DeviceEvent& device)> on_process_move;
    std::function<void(DeviceEvent& device)> on_process_drag;
};

class InputResolverBuilder
{
public:
    using DeviceEvent = mir::shell::decoration::InputResolver::DeviceEvent;
    static auto build(
        std::function<void(DeviceEvent& device)> on_process_enter,
        std::function<void()> on_process_leave,
        std::function<void()> on_process_down,
        std::function<void()> on_process_up,
        std::function<void(DeviceEvent& device)> on_process_move,
        std::function<void(DeviceEvent& device)> on_process_drag) -> InputResolverBuilder;

    auto done() -> std::unique_ptr<InputResolverAdapter>;

private:
    InputResolverBuilder();

    std::unique_ptr<InputResolverAdapter> adapter;
};
}
#endif
