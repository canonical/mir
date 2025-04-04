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

    MirPointerButton waiting_for_button{mir_pointer_button_primary};

    enum class MicroState
    {
        waiting_for_synth_down,
        waiting_for_synth_up,
    } micro_state{MicroState::waiting_for_synth_down};

    enum class MacroState
    {
        waiting_for_left_down,
        synthesizing_left_or_right_click,
        // -> synthesize left click (down, up) if we get an up before the
        // alarm is triggered
        // (waiting_for_button=mir_pointer_button_primary), then back to
        // `waiting_for_left_down`
        //
        // -> synthesize right click (down, up) otherwise
        // (waiting_for_button=mir_pointer_button_secondary), then wait for the
        // user to release their left button
        waiting_for_left_up
    } macro_state{MacroState::waiting_for_left_down};
};
}
}

#endif // MIR_SHELL_SIMULATED_SECONDARY_CLICK_TRANSFORMER_H
