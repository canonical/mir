/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_GRAPHICS_MULTI_DISPLAY_CLOCK_H_
#define MIR_GRAPHICS_MULTI_DISPLAY_CLOCK_H_

#include "display_clock.h"
#include <functional>
#include <memory>
#include <vector>
#include <mutex>

namespace mir
{
namespace graphics
{

/**
 * MultiDisplayClock is a virtual display clock that can represent any
 * number of child clocks. It ticks at the rate of the fastest child,
 * providing the user (and hence client app) a single clock to sync to.
 */
class MultiDisplayClock : public DisplayClock
{
public:
    virtual ~MultiDisplayClock() = default;
    Frame last_frame() const override;
    void set_frame_callback(FrameCallback const&) override;
    void add_child_clock(std::weak_ptr<DisplayClock>);
private:
    typedef std::lock_guard<std::mutex> Lock;
    mutable std::mutex mutex;

    void synchronize(Lock const&);
    void hook_child_clock(Lock const&, DisplayClock& child_clock, int idx);
    void on_child_frame(int child_index, Frame const&);

    struct Child
    {
        std::weak_ptr<DisplayClock> clock;
        Frame baseline;
    };
    std::vector<Child> children;
    FrameCallback callback;
    Frame baseline;
    Frame last_multi_frame;
};

}
}

#endif // MIR_GRAPHICS_MULTI_DISPLAY_CLOCK_H_
