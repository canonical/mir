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
#include <functional>

namespace mir { namespace graphics {

typedef std::function<void(Frame const&)> FrameCallback;

/**
 * Output is a generic output abstraction for unifying common traits of
 * physical outputs, cloned monitor groups, and display buffers through
 * the stack.
 *
 * Its present purpose is to provide access to Frame information. In future
 * it may also provide other grouped information such as the best recommended
 * sub-pixel arrangement to use. Such information is dictated by physical
 * outputs, but needs to be known at a much higher level during rendering.
 */
class Output
{
public:
    virtual ~Output() = default;
    virtual void set_frame_callback(FrameCallback const&) = 0;
};

}} // namespace mir::graphics

#endif // MIR_GRAPHICS_OUTPUT_H_
