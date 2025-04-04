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

#ifndef MIR_SHELL_SIMULATED_SECONDARY_CLICK_TRANSFORMER_H
#define MIR_SHELL_SIMULATED_SECONDARY_CLICK_TRANSFORMER_H

#include "mir/input/input_event_transformer.h"

#include "mir/time/alarm.h"

namespace mir
{
class MainLoop;
namespace shell
{
class SimulatedSecondaryClickTransformer : public mir::input::InputEventTransformer::Transformer
{
public:
    SimulatedSecondaryClickTransformer(std::shared_ptr<mir::MainLoop> const& main_loop);

    bool transform_input_event(
        input::InputEventTransformer::EventDispatcher const& dispatcher,
        input::EventBuilder*,
        MirEvent const&) override;

private:
    std::shared_ptr<mir::MainLoop> const main_loop;
    std::unique_ptr<time::Alarm> secondary_click_dispatcher;

    enum class State
    {
        waiting_for_real_left_down,

        waiting_for_synthesized_right_down,
        waiting_for_synthesized_right_up,

        waiting_for_real_left_up,

        waiting_for_synthesized_left_down,
        waiting_for_synthesized_left_up,
    } state{State::waiting_for_real_left_down};
};
}
}

#endif // MIR_SHELL_SIMULATED_SECONDARY_CLICK_TRANSFORMER_H
