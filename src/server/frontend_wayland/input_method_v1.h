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

#include "input-method-unstable-v1_wrapper.h"
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

class InputMethodV1 : public wayland::InputMethodV1::Global
{
public:
    InputMethodV1(
        wl_display* display,
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<scene::TextInputHub> const& text_input_hub);

private:
    class Instance;
    void bind(wl_resource *new_resource) override;

    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<scene::TextInputHub> const text_input_hub;
};

/// Interface for implementing virtual keyboards.
class InputPanelV1 : public wayland::InputPanelV1::Global
{
public:
    InputPanelV1(
        wl_display *display,
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<shell::Shell> const& shell,
        WlSeat* seat,
        OutputManager* output_manager,
        std::shared_ptr<scene::TextInputHub> const& text_input_hub);

private:
    class Instance;
    void bind(wl_resource* new_zwp_input_panel_v1) override;

    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<shell::Shell> const shell;
    WlSeat* seat;
    OutputManager* const output_manager;
    std::shared_ptr<scene::TextInputHub> const text_input_hub;
};

}
}

#endif
