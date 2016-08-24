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

#ifndef MIR_GRAPHICS_ESTIMATE_FRAME_H_
#define MIR_GRAPHICS_ESTIMATE_FRAME_H_

#include "mir/graphics/atomic_frame.h"

namespace mir { namespace graphics {

/**
 * EstimateFrame provides frame counting for platforms that lack some frame
 * counting or timestamp features. The result of using EstimateFrame should
 * be accurate enough for production use but is less ideal than using a
 * pure AtomicFrame populated directly by the graphics driver.
 */
class EstimateFrame : public AtomicFrame
{
public:
    void increment_now();
    void increment_with_timestamp(Timestamp t);
};

}} // namespace mir::graphics

#endif // MIR_GRAPHICS_ESTIMATE_FRAME_H_
