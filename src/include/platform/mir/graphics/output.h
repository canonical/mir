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

#ifndef MIR_GRAPHICS_OUTPUT_H_
#define MIR_GRAPHICS_OUTPUT_H_

#include "mir/graphics/frame.h"

namespace mir { namespace graphics {

/**
 * Output is a generic output abstraction for unifying attributes of
 * physical outputs to a high level where they are required in rendering.
 * Such attibutes of physical outputs required during rendering are:
 *   - Frame timing information
 *   - Sub-pixel RGB arrangement (future enhancement)
 *   - Resolution (future enhancement?)
 * Although we usually like to abstract low-level implementation details,
 * such attributes of the physical display need to be known at a high level
 * in order to render with optimal visual quality and precision.
 */
class Output
{
public:
    virtual ~Output() = default;
    virtual Frame last_frame() const = 0;
};

}} // namespace mir::graphics

#endif // MIR_GRAPHICS_OUTPUT_H_
