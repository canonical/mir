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
#include "client.h"
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

class InputMethodV1
{
public:
    InputMethodV1(std::shared_ptr<scene::TextInputHub> const& text_input_hub,
         std::shared_ptr<Executor> const& wayland_executor);
    auto create() -> std::shared_ptr<wayland_rs::ZwpInputMethodV1Impl>;

private:
    class Instance;

    std::shared_ptr<scene::TextInputHub> text_input_hub;
    std::shared_ptr<Executor> wayland_executor;
};

/// Interface for implementing virtual keyboards.
class InputPanelV1
{
public:
    InputPanelV1(
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<shell::Shell> const& shell,
        WlSeat* seat,
        OutputManager* output_manager,
        std::shared_ptr<scene::TextInputHub> const& text_input_hub,
        std::shared_ptr<SurfaceRegistry> const& surface_registry);

    auto create(std::shared_ptr<wayland_rs::Client> const& client) -> std::shared_ptr<wayland_rs::ZwpInputPanelV1Impl>;

private:
    class Instance;

    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<shell::Shell> const shell;
    WlSeat* seat;
    OutputManager* const output_manager;
    std::shared_ptr<scene::TextInputHub> const text_input_hub;
    std::shared_ptr<SurfaceRegistry> const surface_registry;
};

}
}

#endif
