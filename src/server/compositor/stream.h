/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_COMPOSITOR_STREAM_H_
#define MIR_COMPOSITOR_STREAM_H_

#include "mir/compositor/buffer_stream.h"
#include "mir/scene/surface_observers.h"
#include "mir/frontend/buffer_stream_id.h"
#include "mir/lockable_callback.h"
#include "mir/geometry/size.h"
#include "multi_monitor_arbiter.h"
#include <mutex>
#include <memory>
#include <set>

namespace mir
{
namespace frontend { class ClientBuffers; }
namespace compositor
{
class Schedule;
class Stream : public BufferStream
{
public:
    Stream(geometry::Size sz, MirPixelFormat format);
    ~Stream();

    void submit_buffer(std::shared_ptr<graphics::Buffer> const& buffer) override;
    void with_most_recent_buffer_do(std::function<void(graphics::Buffer&)> const& exec) override;
    MirPixelFormat pixel_format() const override;
    void set_frame_posted_callback(
        std::function<void(geometry::Size const&)> const& callback) override;
    std::shared_ptr<graphics::Buffer>
        lock_compositor_buffer(void const* user_id) override;
    geometry::Size stream_size() override;
    void resize(geometry::Size const& size) override;
    void allow_framedropping(bool) override;
    bool framedropping() const override;
    int buffers_ready_for_compositor(void const* user_id) const override;
    void drop_old_buffers() override;
    bool has_submitted_buffer() const override;
    void set_scale(float scale) override;

private:
    enum class ScheduleMode;
    void transition_schedule(std::shared_ptr<Schedule>&& new_schedule, std::lock_guard<std::mutex> const&);

    std::mutex mutable mutex;
    ScheduleMode schedule_mode;
    std::shared_ptr<Schedule> schedule;
    std::shared_ptr<MultiMonitorArbiter> const arbiter;
    geometry::Size size; 
    MirPixelFormat pf;
    bool first_frame_posted;

    std::mutex callback_mutex;
    std::function<void(geometry::Size const&)> frame_callback;
};
}
}

#endif /* MIR_COMPOSITOR_STREAM_H_ */
