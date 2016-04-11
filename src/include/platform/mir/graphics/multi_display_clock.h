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
#include <set>

namespace mir
{
namespace graphics
{

class MultiDisplayClock : public DisplayClock
{
public:
    virtual ~MultiDisplayClock() = default;
    Frame last_frame() const override;
    void on_next_frame(FrameCallback const&) override;
    void add_child_clock(std::weak_ptr<DisplayClock>);
private:
    void synchronize();
    void on_child_frame(Frame const&);
    std::set<std::weak_ptr<DisplayClock>> children;
    FrameCallback cb;
};

}
}

#endif // MIR_GRAPHICS_MULTI_DISPLAY_CLOCK_H_
