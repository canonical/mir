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

#ifndef MIR_GRAPHICS_SIMPLE_OUTPUT_H_
#define MIR_GRAPHICS_SIMPLE_OUTPUT_H_

#include "mir/graphics/output.h"
#include <mutex>

namespace mir { namespace graphics {

class SimpleOutput : virtual public Output
{
public:
    virtual ~SimpleOutput() = default;
    virtual void set_frame_callback(FrameCallback const&) override;
protected:
    void notify_frame(Frame const&);
private:
    typedef std::mutex Mutex;
    mutable Mutex mutex;
    FrameCallback callback;
};

}} // namespace mir::graphics

#endif // MIR_GRAPHICS_SIMPLE_OUTPUT_H_
