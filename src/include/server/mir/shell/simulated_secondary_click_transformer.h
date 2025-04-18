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

namespace mir
{
namespace shell
{
class SimulatedSecondaryClickTransformer : public mir::input::InputEventTransformer::Transformer
{
public:
    virtual void hold_duration(std::chrono::milliseconds delay) = 0;
    virtual void displacement_threshold(float displacement) = 0;
    virtual void enabled() = 0;
    virtual void disabled() = 0;
    virtual void enabled(std::function<void()>&& on_enabled) = 0;
    virtual void disabled(std::function<void()>&& on_disabled) = 0;
    virtual void hold_start(std::function<void()>&& on_hold_start) = 0;
    virtual void hold_cancel(std::function<void()>&& on_hold_cancel) = 0;
    virtual void secondary_click(std::function<void()>&& on_secondary_click) = 0;
};
}
}

#endif // MIR_SHELL_SIMULATED_SECONDARY_CLICK_TRANSFORMER_H
