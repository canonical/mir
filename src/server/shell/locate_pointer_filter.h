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

#ifndef MIR_SHELL_LOCATE_POINTER_FILTER_H_
#define MIR_SHELL_LOCATE_POINTER_FILTER_H_

#include "mir/geometry/point.h"

#include "mir/input/event_filter.h"
#include "mir/time/alarm.h"

#include <chrono>
#include <functional>
#include <memory>

namespace mir
{
class MainLoop;
namespace shell
{
class LocatePointerFilter : public input::EventFilter
{
public:
    struct State
    {
        std::function<void(float, float)> on_locate_pointer{[](auto, auto)
                                                            {
                                                            }};
        std::chrono::milliseconds delay{500};
        mir::geometry::PointF cursor_position{0.0f, 0.0f}; // Assumes the cursor always starts at (0, 0)
    };

    LocatePointerFilter(std::shared_ptr<mir::MainLoop> const main_loop, State& initial_state);

    auto handle(MirEvent const& event) -> bool override;

private:
    auto handle_ctrl(MirInputEvent const* input_event) -> bool;
    void record_pointer_position(MirInputEvent const* input_event);

    State& state;
    std::shared_ptr<mir::MainLoop> const main_loop;
    std::unique_ptr<time::Alarm> const locate_pointer_alarm;
};
}
}

#endif // MIR_SHELL_LOCATE_POINTER_FILTER_H_
