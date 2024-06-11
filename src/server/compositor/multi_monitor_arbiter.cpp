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
#include "mir/compositor/buffer_stream.h"
#include "mir/geometry/forward.h"
#include "mir/graphics/buffer.h"
#include "mir/frontend/event_sink.h"
#include "mir/graphics/drm_formats.h"
#include "multi_threaded_compositor.h"
#include <boost/throw_exception.hpp>
#include <algorithm>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mf = mir::frontend;

namespace geom = mir::geometry;

struct mc::MultiMonitorArbiter::Submission
{
    std::shared_ptr<mg::Buffer> buffer;
    geom::Size output_size;
    geom::RectangleD source_sample;
};

namespace
{
class TrackingSubmission : public mc::BufferStream::Submission
{
public:
    TrackingSubmission(
        std::shared_ptr<mc::MultiMonitorArbiter::Submission> submission,
        std::function<void()> on_claimed)
        : submission{std::move(submission)},
          on_claimed{std::move(on_claimed)}
    {
    }

    auto claim_buffer() -> std::shared_ptr<mg::Buffer> override
    {
        on_claimed();
        return submission->buffer;
    }

    auto size() const -> geom::Size override
    {
        return submission->output_size;
    }

    auto source_rect() const -> geom::RectangleD override
    {
        return submission->source_sample;
    }

    auto pixel_format() const -> mg::DRMFormat override
    {
        return mg::DRMFormat::from_mir_format(submission->buffer->pixel_format());
    }
private:
    std::shared_ptr<mc::MultiMonitorArbiter::Submission> submission;
    std::function<void()> on_claimed;
};
}

mc::MultiMonitorArbiter::MultiMonitorArbiter()
{
    // We're highly unlikely to have more than 6 outputs
    state.lock()->current_buffer_users.reserve(6);
}

mc::MultiMonitorArbiter::~MultiMonitorArbiter()
{
}

auto mc::MultiMonitorArbiter::compositor_acquire(compositor::CompositorID id)
    -> std::shared_ptr<BufferStream::Submission>
{
    auto current_state = state.lock();

    // If there is no current buffer or there is, but this compositor is already using it...
    if (!current_state->current_submission || is_user_of_current_buffer(*current_state, id))
    {
        // And if there is a scheduled buffer
        if (current_state->next_submission)
        {
            // Advance the current buffer
            current_state->current_submission = std::move(current_state->next_submission);
            current_state->next_submission = nullptr;
            clear_current_users(*current_state);
        }
        // Otherwise leave the current buffer alone
    }

    // If there was no current buffer and we weren't able to set one, throw and exception
    if (!current_state->current_submission)
        BOOST_THROW_EXCEPTION(std::logic_error("no buffer to give to compositor"));

    return std::make_shared<TrackingSubmission>(
        current_state->current_submission,
        [me = shared_from_this(), submission = current_state->current_submission, id]()
        {
            auto state = me->state.lock();
            // Ensure we still have the same state
            if (state->current_submission != submission)
                return;
            // The compositor is now a user of the current buffer
            // This means we will try to give it a new buffer next time it asks
            add_current_buffer_user(*state, id);
        });
}

void mc::MultiMonitorArbiter::submit_buffer(
    std::shared_ptr<mg::Buffer> buffer,
    geom::Size output_size,
    geom::RectangleD source)
{
    state.lock()->next_submission = std::make_shared<Submission>(std::move(buffer), output_size, source);
}

bool mc::MultiMonitorArbiter::buffer_ready_for(mc::CompositorID id)
{
    auto current_state = state.lock();
    // If there are scheduled buffers then there is one ready for any compositor
    if (current_state->next_submission)
        return true;
    // If we have a current buffer that the compositor isn't yet using, it is ready
    else if (current_state->current_submission && !is_user_of_current_buffer(*current_state, id))
        return true;
    // There are no scheduled buffers and either no current buffer, or a current buffer already used by this compositor
    else
        return false;
}

void mc::MultiMonitorArbiter::add_current_buffer_user(State& state, mc::CompositorID id)
{
    // First try and find an empty slot in our vector…
    for (auto& slot : state.current_buffer_users)
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
    state.current_buffer_users.push_back({id});
}

bool mc::MultiMonitorArbiter::is_user_of_current_buffer(State& state, mir::compositor::CompositorID id)
{
    return std::any_of(
        state.current_buffer_users.begin(),
        state.current_buffer_users.end(),
        [id](auto const& slot)
        {
            if (slot)
            {
                return *slot == id;
            }
            return false;
        });
}

void mc::MultiMonitorArbiter::clear_current_users(State& state)
{
    for (auto& slot : state.current_buffer_users)
    {
        slot = {};
    }
}
