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

#include "threaded_drm_event_handler.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/enable_error_info.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/exception/info.hpp>

#include <xf86drmMode.h>
#include <xf86drm.h>
#include <poll.h>
#include <cstring>
#include <algorithm>

#define MIR_LOG_COMPONENT "EGLStream KMS event handler"
#include "mir/log.h"

namespace mge = mir::graphics::eglstream;

mge::ThreadedDRMEventHandler::ThreadedDRMEventHandler(mir::Fd drm_fd)
    : drm_fd{std::move(drm_fd)},
      dispatch_thread{[this]() { event_loop(); }}
{

}

mge::ThreadedDRMEventHandler::~ThreadedDRMEventHandler()
{
    {
        std::lock_guard<std::mutex> lock{expectation_mutex};
        shutdown = true;
        expectations_changed.notify_all();
    }
    if (dispatch_thread.joinable())
    {
        dispatch_thread.join();
    }
}

void const* mge::ThreadedDRMEventHandler::drm_event_data() const
{
    return &pending_expectations;
}

namespace
{
class NotifyOnScopeExit
{
public:
    NotifyOnScopeExit(std::condition_variable& notifier)
        : notifier{notifier}
    {
    }

    ~NotifyOnScopeExit()
    {
        notifier.notify_all();
    }
private:
    std::condition_variable& notifier;
};
}

std::future<void> mge::ThreadedDRMEventHandler::expect_flip_event(
    DRMEventHandler::KMSCrtcId id,
    std::function<void(unsigned int frame_number, std::chrono::milliseconds frame_time)> on_flip)
{
    NotifyOnScopeExit notifier{expectations_changed};
    std::lock_guard<std::mutex> lock{expectation_mutex};
    // First check if there's an empty slot in the vector (there probably is)
    for (auto& slot : pending_expectations)
    {
        if (!slot)
        {
            slot = FlipEventData {
                id,
                std::move(on_flip),
                std::promise<void>{}
            };
            return slot->completion.get_future();
        }
    }
    // There isn't, we'll need to expand the vector…
    pending_expectations.emplace_back(FlipEventData {id, std::move(on_flip), std::promise<void>{}});
    return pending_expectations.back()->completion.get_future();
}

void mge::ThreadedDRMEventHandler::event_loop() noexcept
{
    drmEventContext ctx;
    ::memset(&ctx, 0, sizeof(ctx));
    ctx.version = 3;
    ctx.page_flip_handler2 = &flip_handler;

    auto const any_pending_expectations =
        [this]()
        {
            return std::any_of(
                pending_expectations.begin(),
                pending_expectations.end(),
                [](auto const& slot) { return static_cast<bool>(slot); });
        };

    std::unique_lock<std::mutex> lock{expectation_mutex};
    while (!shutdown)
    {
        if (!any_pending_expectations())
        {
            expectations_changed.wait(
                lock,
                [this, &any_pending_expectations]()
                {
                    return shutdown || any_pending_expectations();
                });
        }
        if (shutdown)
        {
            return;
        }

        /*
         * We have some pending expectations.
         * Drop the lock (consumers can only add expectations at this point) and wait for
         * the DRM events
         */
        lock.unlock();

        pollfd events;
        events.fd = drm_fd;
        events.events = POLLIN;
        // TODO: Maybe have a non-infinite timeout here?
        auto result = ::poll(&events, 1, -1);

        // We've got some events; reclaim the lock and process them.
        lock.lock();
        if (result == -1 && errno != EINTR)
        {
            // Error: abandon all the pending flips, then wait for further instructions
            for (auto& slot : pending_expectations)
            {
                if (slot)
                {
                    slot->completion.set_exception(
                        std::make_exception_ptr(
                            boost::enable_error_info(
                                std::system_error{
                                    errno,
                                    std::system_category(),
                                    "Error waiting for DRM event"})
                                << boost::throw_file(__FILE__)
                                << boost::throw_line(__LINE__)
                                << boost::throw_function(__PRETTY_FUNCTION__)));
                    slot = {};
                }
            }
        }
        else if (events.revents & POLLIN)
        {
            drmHandleEvent(drm_fd, &ctx);
        }
    }
}

void mge::ThreadedDRMEventHandler::flip_handler(
    int /*drm_fd*/,
    unsigned int frame_number,
    unsigned int sec,
    unsigned int usec,
    KMSCrtcId crtc_id,
    void* data) noexcept
{
    /*
     * No need to lock (and indeed, locking would be incorrect) as this is only called from
     * drmHandleEvent() which is only called from event_loop, and is called while holding the lock
     */
    auto pending_expectations = static_cast<std::vector<std::experimental::optional<FlipEventData>>*>(data);
    for (auto& slot : *pending_expectations)
    {
        if (slot && slot->id == crtc_id)
        {
            slot->callback(
                frame_number,
                std::chrono::seconds{sec} + std::chrono::milliseconds{usec});
            slot->completion.set_value();
            slot = {};
            return;
        }
    }
}
