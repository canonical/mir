/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_TOUCH_VISUALIZER_H_
#define MIR_TEST_DOUBLES_STUB_TOUCH_VISUALIZER_H_

#include "mir/input/touch_visualizer.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubTouchVisualizer : public input::TouchVisualizer
{
    void visualize_touches(std::vector<Spot> const&) override
    {
    }
    void enable() override
    {
    }
    void disable() override
    {
    }
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_TOUCH_VISUALIZER_H_

