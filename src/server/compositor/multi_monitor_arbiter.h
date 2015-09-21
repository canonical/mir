/*
 * Copyright © 2015 Canonical Ltd.
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
 */

#ifndef MIR_COMPOSITOR_MULTI_MONITOR_ARBITER_H_
#define MIR_COMPOSITOR_MULTI_MONITOR_ARBITER_H_

#include "mir/compositor/compositor_id.h"
#include "mir/graphics/buffer_id.h"
#include "buffer_bundle.h"
#include <memory>
#include <mutex>
#include <deque>
#include <set>

namespace mir
{
namespace graphics { class Buffer; }
namespace frontend { class ClientBuffers; }
namespace compositor
{
class Schedule;

enum class MultiMonitorMode
{
    multi_monitor_sync,
    single_monitor_fast
};

class MultiMonitorArbiter : public BufferAcquisition 
{
public:
    MultiMonitorArbiter(
        MultiMonitorMode mode,
        std::shared_ptr<frontend::ClientBuffers> const& map,
        std::shared_ptr<Schedule> const& schedule);

    std::shared_ptr<graphics::Buffer> compositor_acquire(compositor::CompositorID id) override;
    void compositor_release(std::shared_ptr<graphics::Buffer> const&) override;
    std::shared_ptr<graphics::Buffer> snapshot_acquire() override;
    void snapshot_release(std::shared_ptr<graphics::Buffer> const&) override;
    void set_schedule(std::shared_ptr<Schedule> const& schedule);
    void set_mode(MultiMonitorMode mode);
    bool buffer_ready_for(compositor::CompositorID id);

private:
    void decrease_refcount_for(graphics::BufferID id, std::lock_guard<std::mutex> const&);
    void clean_onscreen_buffers(std::lock_guard<std::mutex> const&);

    std::mutex mutable mutex;
    MultiMonitorMode mode;
    std::shared_ptr<frontend::ClientBuffers> const map;
    struct ScheduleEntry
    {
        ScheduleEntry(std::shared_ptr<graphics::Buffer> const& buffer, unsigned int use_count) :
            buffer(buffer),
            use_count(use_count)
        {
        }
        std::shared_ptr<graphics::Buffer> buffer;
        unsigned int use_count;
    };
    std::deque<ScheduleEntry> onscreen_buffers;
    std::set<compositor::CompositorID> current_buffer_users;
    std::shared_ptr<Schedule> schedule;
};

}
}
#endif /* MIR_COMPOSITOR_MULTI_MONITOR_ARBITER_H_ */
