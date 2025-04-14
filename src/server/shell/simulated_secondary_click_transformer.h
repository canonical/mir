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

#include "mir/synchronised.h"
#include "mir/time/alarm.h"

#include <atomic>

namespace mir
{
namespace shell
{
class SimulatedSecondaryClickTransformer : public mir::input::InputEventTransformer::Transformer
{
public:
    virtual void hold_duration(std::chrono::milliseconds delay) = 0;
    virtual void hold_start(std::function<void()>&& on_hold_start) = 0;
    virtual void hold_cancel(std::function<void()>&& on_hold_cancel) = 0;
    virtual void secondary_click(std::function<void()>&& on_secondary_click) = 0;
};

class BasicSimulatedSecondaryClickTransformer : public SimulatedSecondaryClickTransformer
{
public:
    BasicSimulatedSecondaryClickTransformer(
        std::shared_ptr<mir::MainLoop> const& main_loop, MirInputDeviceId virtual_device_id);

    bool transform_input_event(
        input::InputEventTransformer::EventDispatcher const& dispatcher,
        input::EventBuilder*,
        MirEvent const&) override;

    void hold_duration(std::chrono::milliseconds delay) override;
    void hold_start(std::function<void()>&& on_hold_start) override;
    void hold_cancel(std::function<void()>&& on_hold_cancel) override;
    void secondary_click(std::function<void()>&& on_secondary_click) override;

private:
    std::shared_ptr<mir::MainLoop> const main_loop;
    MirInputDeviceId const virtual_device_id;
    std::unique_ptr<time::Alarm> secondary_click_dispatcher;

    enum class State
    {
        waiting_for_real_left_down,
        waiting_for_motion_or_real_left_up,
        waiting_for_drag_end_left_up,
    } state{State::waiting_for_real_left_down};

    struct MutableState
    {
        std::chrono::milliseconds hold_duration{1000};

        auto static inline constexpr do_nothing = []
        {
        };

        std::function<void()> on_hold_start{do_nothing}, on_hold_cancel{do_nothing}, on_secondary_click{do_nothing};
    };

    mir::Synchronised<MutableState> mutable_state;
};
}
}

#endif // MIR_SHELL_SIMULATED_SECONDARY_CLICK_TRANSFORMER_H
