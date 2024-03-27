/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "multi_monitor_arbiter.h"
#include "mir/graphics/buffer.h"
#include "mir/frontend/event_sink.h"
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mf = mir::frontend;

mc::MultiMonitorArbiter::MultiMonitorArbiter()
{
    // We're highly unlikely to have more than 6 outputs
    current_buffer_users.reserve(6);
}

mc::MultiMonitorArbiter::~MultiMonitorArbiter()
{
}

std::shared_ptr<mg::Buffer> mc::MultiMonitorArbiter::compositor_acquire(compositor::CompositorID id)
{
    std::lock_guard lk(mutex);

    // If there is no current buffer or there is, but this compositor is already using it...
    if (!current_buffer || is_user_of_current_buffer(id))
    {
        // And if there is a scheduled buffer
        if (next_buffer)
        {
            // Advance the current buffer
            current_buffer = std::move(next_buffer);
            next_buffer = nullptr;
            clear_current_users();
        }
        // Otherwise leave the current buffer alone
    }

    // If there was no current buffer and we weren't able to set one, throw and exception
    if (!current_buffer)
        BOOST_THROW_EXCEPTION(std::logic_error("no buffer to give to compositor"));

    // The compositor is now a user of the current buffer
    // This means we will try to give it a new buffer next time it asks
    add_current_buffer_user(id);
    return current_buffer;
}

std::shared_ptr<mg::Buffer> mc::MultiMonitorArbiter::snapshot_acquire()
{
    std::lock_guard lk(mutex);

    if (!current_buffer)
    {
        if (next_buffer)
        {
            current_buffer = std::move(next_buffer);
            next_buffer = nullptr;
            clear_current_users();
        }
        else
        {
            BOOST_THROW_EXCEPTION(std::logic_error("no buffer to give to snapshotter"));
        }
    }

    return current_buffer;
}

void mc::MultiMonitorArbiter::submit_buffer(std::shared_ptr<mg::Buffer> buffer)
{
    std::lock_guard lk{mutex};
    next_buffer.swap(buffer);
}

bool mc::MultiMonitorArbiter::buffer_ready_for(mc::CompositorID id)
{
    std::lock_guard lk(mutex);
    // If there are scheduled buffers then there is one ready for any compositor
    if (next_buffer)
        return true;
    // If we have a current buffer that the compositor isn't yet using, it is ready
    else if (current_buffer && !is_user_of_current_buffer(id))
        return true;
    // There are no scheduled buffers and either no current buffer, or a current buffer already used by this compositor
    else
        return false;
}

void mc::MultiMonitorArbiter::add_current_buffer_user(mc::CompositorID id)
{
    // First try and find an empty slot in our vector…
    for (auto& slot : current_buffer_users)
    {
        if (slot == id)
        {
            return;
        }
        else if (!slot)
        {
            slot = id;
            return;
        }
    }
    //…no empty slot, so we'll need to grow the vector.
    current_buffer_users.push_back({id});
}

bool mc::MultiMonitorArbiter::is_user_of_current_buffer(mir::compositor::CompositorID id)
{
    return std::any_of(
        current_buffer_users.begin(),
        current_buffer_users.end(),
        [id](auto const& slot)
        {
            if (slot)
            {
                return *slot == id;
            }
            return false;
        });
}

void mc::MultiMonitorArbiter::clear_current_users()
{
    for (auto& slot : current_buffer_users)
    {
        slot = {};
    }
}
