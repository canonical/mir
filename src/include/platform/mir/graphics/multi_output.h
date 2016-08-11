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

#ifndef MIR_GRAPHICS_MULTI_OUTPUT_H_
#define MIR_GRAPHICS_MULTI_OUTPUT_H_

#include "mir/graphics/output.h"
#include <functional>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace mir { namespace graphics {

/**
 * MultiOutput implements multi-monitor frame synchronization. This means
 * if you have multiple outputs, MultiOutput will emit frame callbacks
 * at the speed of the fastest one.
 */
class MultiOutput : public Output
{
public:
    virtual ~MultiOutput() = default;
    void add_child_output(std::weak_ptr<Output>);
    virtual Frame last_frame() const override;
private:
    typedef std::mutex Mutex;
    typedef std::lock_guard<Mutex> Lock;
    typedef void const* ChildId;

    void synchronize(Lock const&);

    struct Child
    {
        std::weak_ptr<Output> output;
        Frame last_sync;
        Frame last_frame;
        Frame contributed_to_multi_frame;
    };

    Mutex mutex;
    std::unordered_map<ChildId, Child> children;
    Frame last_sync;
    Frame last_multi_frame;
};

}} // namespace mir::graphics

#endif // MIR_GRAPHICS_MULTI_OUTPUT_H_
