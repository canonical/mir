/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_PLATFORM_EGLSTREAM_THREADED_DRM_EVENT_HANDLER_H_
#define MIR_PLATFORM_EGLSTREAM_THREADED_DRM_EVENT_HANDLER_H_

#include "drm_event_handler.h"

#include "mir/fd.h"

#include <functional>
#include <vector>
#include <experimental/optional>
#include <mutex>
#include <condition_variable>

namespace mir
{
namespace graphics
{
namespace eglstream
{
class ThreadedDRMEventHandler : public DRMEventHandler
{
public:
    ThreadedDRMEventHandler(mir::Fd drm_fd);
    ~ThreadedDRMEventHandler() override;

    void const* drm_event_data() const override;

    std::future<void> expect_flip_event(
        KMSCrtcId id,
        std::function<void(unsigned int, std::chrono::milliseconds)> on_flip) override;

private:
    void event_loop() noexcept;

    static void flip_handler(
        int drm_fd,
        unsigned int frame_number,
        unsigned int sec,
        unsigned int usec,
        KMSCrtcId crtc_id,
        void* data) noexcept;

    mir::Fd const drm_fd;

    struct FlipEventData
    {
        KMSCrtcId id;
        std::function<void(unsigned int, std::chrono::milliseconds)> callback;
        std::promise<void> completion;
    };
    // We *could* do something fancy and lock-free, but a basic mutex will suffice for now
    std::mutex expectation_mutex;
    std::condition_variable expectations_changed;
    std::vector<std::experimental::optional<FlipEventData>> pending_expectations;

    bool shutdown{false};
    std::thread dispatch_thread;
};
}
}
}

#endif //MIR_PLATFORM_EGLSTREAM_THREADED_DRM_EVENT_HANDLER_H_
