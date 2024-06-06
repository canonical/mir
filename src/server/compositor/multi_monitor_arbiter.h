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
 *
 */

#ifndef MIR_COMPOSITOR_MULTI_MONITOR_ARBITER_H_
#define MIR_COMPOSITOR_MULTI_MONITOR_ARBITER_H_

#include "mir/compositor/compositor_id.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/geometry/forward.h"
#include "mir/synchronised.h"
#include <memory>
#include <vector>
#include <optional>

namespace mir
{
namespace graphics { class Buffer; }
namespace compositor
{
class Schedule;

class MultiMonitorArbiter : public std::enable_shared_from_this<MultiMonitorArbiter>
{
public:
    MultiMonitorArbiter();
    ~MultiMonitorArbiter();

    auto compositor_acquire(compositor::CompositorID id) -> std::shared_ptr<BufferStream::Submission>;
    bool buffer_ready_for(compositor::CompositorID id);

    void submit_buffer(
        std::shared_ptr<graphics::Buffer> buffer,
        geometry::Size output_size,
        geometry::RectangleD source_sample);

    struct Submission;
private:
    struct State
    {
        std::vector<std::optional<compositor::CompositorID>> current_buffer_users;
        std::shared_ptr<Submission> current_submission;
        std::shared_ptr<Submission> next_submission;
    };
    Synchronised<State> state;

    static void add_current_buffer_user(State& state, compositor::CompositorID id);
    static bool is_user_of_current_buffer(State& state, compositor::CompositorID id);
    static void clear_current_users(State& state);
};

}
}
#endif /* MIR_COMPOSITOR_MULTI_MONITOR_ARBITER_H_ */
