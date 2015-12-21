/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_TOUCH_VISUALIZER_H_
#define MIR_TEST_DOUBLES_MOCK_TOUCH_VISUALIZER_H_

#include "mir/input/touch_visualizer.h"

#include <gmock/gmock.h>

namespace mir
{
namespace input
{
bool operator==(TouchVisualizer::Spot const& lhs, TouchVisualizer::Spot const& rhs)
{
    return lhs.touch_location == rhs.touch_location && lhs.pressure == rhs.pressure;
}
}
namespace test
{
namespace doubles
{

struct MockTouchVisualizer : public input::TouchVisualizer
{
    MOCK_METHOD1(visualize_touches, void(std::vector<input::TouchVisualizer::Spot> const&));
    MOCK_METHOD0(enable, void());
    MOCK_METHOD0(disable, void());
};

}
}
}

#endif
