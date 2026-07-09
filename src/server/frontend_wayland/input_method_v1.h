/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#ifndef MIR_FRONTEND_INPUT_METHOD_V1_H
#define MIR_FRONTEND_INPUT_METHOD_V1_H

#include "input_method_unstable_v1.h"
#include <memory>

namespace mir
{
class Executor;

namespace scene
{
class TextInputHub;
}

namespace shell
{
class Shell;
}

namespace frontend
{
class WlSeat;
class OutputManager;
class SurfaceRegistry;

auto create_zwp_input_method_v1(
    std::shared_ptr<wayland_rs::Client> client,
    rust::Box<wayland_rs::InputMethodV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<scene::TextInputHub> text_input_hub)
-> std::shared_ptr<wayland_rs::InputMethodV1>;

/// Interface for implementing virtual keyboards.
auto create_zwp_input_panel_v1(
    std::shared_ptr<wayland_rs::Client> client,
    rust::Box<wayland_rs::InputPanelV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<shell::Shell> shell,
    WlSeat* seat,
    OutputManager* output_manager,
    std::shared_ptr<scene::TextInputHub> text_input_hub,
    std::shared_ptr<SurfaceRegistry> surface_registry)
-> std::shared_ptr<wayland_rs::InputPanelV1>;
}
}

#endif
