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

#ifndef MIR_INPUT_TOUCH_VISUALIZER_H_
#define MIR_INPUT_TOUCH_VISUALIZER_H_

#include "mir/geometry/point.h"

#include <vector>

namespace mir
{
namespace input
{

/// An interface for listening to a low level stream of touches, in order to provide
// a "spot" style visualization.
class TouchVisualizer
{
public:
    virtual ~TouchVisualizer() = default;

    struct Spot
    {
        geometry::Point touch_location;
        
        // If pressure is non-zero, it indicates a press at the spot, as
        // opposed to a hover.
        float pressure;
    };
    
    virtual void visualize_touches(std::vector<Spot> const& touches) = 0;

protected:
    TouchVisualizer() = default;
    TouchVisualizer(const TouchVisualizer&) = delete;
    TouchVisualizer& operator=(const TouchVisualizer&) = delete;
};

}
}

#endif // MIR_INPUT_TOUCH_VISUALIZER_H_
