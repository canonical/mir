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

namespace mir
{
namespace frontend { class ClientBuffers; }
namespace compositor
{
class Schedule;
class FrameDroppingPolicyFactory;
class FrameDroppingPolicy;
class Stream : public BufferStream
{
public:
    Stream(
        FrameDroppingPolicyFactory const& policy_factory,
        std::shared_ptr<frontend::ClientBuffers>, geometry::Size sz, MirPixelFormat format);

    void swap_buffers(
        graphics::Buffer* old_buffer, std::function<void(graphics::Buffer* new_buffer)> complete) override;
    void with_most_recent_buffer_do(std::function<void(graphics::Buffer&)> const& exec) override;
    MirPixelFormat pixel_format() const override;
    void add_observer(std::shared_ptr<scene::SurfaceObserver> const& observer) override;
    void remove_observer(std::weak_ptr<scene::SurfaceObserver> const& observer) override;
    std::shared_ptr<graphics::Buffer>
        lock_compositor_buffer(void const* user_id) override;
    geometry::Size stream_size() override;
    void resize(geometry::Size const& size) override;
    void allow_framedropping(bool) override;
    void drop_outstanding_requests() override;
    int buffers_ready_for_compositor(void const* user_id) const override;
    void drop_old_buffers() override;
    bool has_submitted_buffer() const override;
    void set_scale(float scale) override;

private:
    enum class ScheduleMode;
    struct DroppingCallback : mir::LockableCallback
    {
        DroppingCallback(Stream* stream);
        void operator()() override;
        void lock() override;
        void unlock() override;
        Stream* stream;
        std::unique_lock<std::mutex> guard_lock;
    };
    void transition_schedule(std::shared_ptr<Schedule>&& new_schedule, std::lock_guard<std::mutex> const&);
    void drop_frame();

    std::mutex mutable mutex;
    std::unique_ptr<compositor::FrameDroppingPolicy> drop_policy;
    ScheduleMode schedule_mode;
    std::shared_ptr<Schedule> schedule;
    std::shared_ptr<frontend::ClientBuffers> buffers;
    std::shared_ptr<MultiMonitorArbiter> arbiter;
    geometry::Size size; 
    MirPixelFormat const pf;
    bool first_frame_posted;

    scene::SurfaceObservers observers;
};
}
}

#endif /* MIR_COMPOSITOR_STREAM_H_ */
