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

#include "mir/graphics/multi_output.h"

namespace mir { namespace graphics {

void MultiOutput::add_child_output(std::weak_ptr<Output> w)
{
    Lock lock(mutex);
    if (auto output = w.lock())
    {
        ChildId id = output.get();
        children[id].output = std::move(w);
        synchronize(lock);
        /* TODO
        output->set_frame_callback(
            std::bind(&MultiOutput::on_child_frame,
                      this, id, std::placeholders::_1) );
        */
    }
}

void MultiOutput::synchronize(Lock const&)
{
    last_sync = last_multi_frame;

    auto c = children.begin();
    while (c != children.end())
    {
        auto& child = c->second;
        if (child.output.expired())
        {
            /*
             * Lazy deferred clean-up. We don't need to do this any sooner
             * because a deleted child (which no longer generates callbacks)
             * doesn't affect results at all. This means we don't need to
             * ask graphics platforms to tell us when an output is removed.
             */
            c = children.erase(c);
        }
        else
        {
            child.last_sync = child.last_frame;
            ++c;
        }
    }
}

Frame MultiOutput::last_frame() const
{
    // TODO
    return {};
}

}} // namespace mir::graphics
