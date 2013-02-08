/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "kms_page_flip_manager.h"
#include "mir/graphics/display_listener.h"

#include <limits>
#include <stdexcept>
#include <boost/throw_exception.hpp>

#include <xf86drm.h>
#include <xf86drmMode.h>

namespace mgg = mir::graphics::gbm;

namespace
{

struct timeval chrono_to_timeval(std::chrono::microseconds us)
{
    namespace sc = std::chrono;

    auto s = sc::duration_cast<sc::seconds>(us);
    auto us_left = sc::duration_cast<sc::microseconds>(us - s);

    if (s.count() > std::numeric_limits<decltype(timeval::tv_sec)>::max() ||
        s.count() < 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to convert microseconds to timeval, overflow in tv_sec"));
    }

    if (us_left.count() > std::numeric_limits<decltype(timeval::tv_usec)>::max() ||
        us_left.count() < 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to convert microseconds to timeval, overflow in tv_usec"));
    }

    return {static_cast<decltype(timeval::tv_sec)>(s.count()),
            static_cast<decltype(timeval::tv_usec)>(us_left.count())};
}

void page_flip_handler(int /*fd*/, unsigned int /*frame*/,
                       unsigned int /*sec*/, unsigned int /*usec*/,
                       void* data)
{
    auto page_flip_data = static_cast<mgg::PageFlipEventData*>(data);
    page_flip_data->pending->erase(page_flip_data->crtc_id);
}

}

mgg::KMSPageFlipManager::KMSPageFlipManager(int drm_fd,
                                            std::chrono::microseconds max_wait,
                                            std::shared_ptr<DisplayListener> const& listener)
    : drm_fd{drm_fd},
      tv_page_flip_max_wait(chrono_to_timeval(max_wait)),
      listener(listener),
      pending_page_flips(),
      worker_tid()
{
}

bool mgg::KMSPageFlipManager::schedule_page_flip(uint32_t crtc_id, uint32_t fb_id)
{
    std::unique_lock<std::mutex> lock{pf_mutex};

    if (pending_page_flips.find(crtc_id) != pending_page_flips.end())
        BOOST_THROW_EXCEPTION(std::logic_error("Page flip for crtc_id is already scheduled"));

    pending_page_flips[crtc_id] = PageFlipEventData{&pending_page_flips, crtc_id};

    auto ret = drmModePageFlip(drm_fd, crtc_id, fb_id,
                               DRM_MODE_PAGE_FLIP_EVENT,
                               &pending_page_flips[crtc_id]);

    if (ret)
        pending_page_flips.erase(crtc_id);

    return (ret == 0);
}

void mgg::KMSPageFlipManager::wait_for_page_flip(uint32_t crtc_id)
{
    static drmEventContext evctx =
    {
        DRM_EVENT_CONTEXT_VERSION,  /* .version */
        0,  /* .vblank_handler */
        page_flip_handler  /* .page_flip_handler */
    };
    static std::thread::id const invalid_tid;

    {
        std::unique_lock<std::mutex> lock{pf_mutex};

        /*
         * While another thread is the worker (it is controlling the
         * page flip event loop) and our event has not arrived, wait.
         */
        while (worker_tid != invalid_tid && !page_flip_is_done(crtc_id))
            pf_cv.wait(lock);

        /* If the page flip we are waiting for has arrived we are done. */
        if (page_flip_is_done(crtc_id))
            return;

        /* ...otherwise we become the worker */
        worker_tid = std::this_thread::get_id();
    }

    /* Only the worker thread reaches this point */
    bool done{false};

    while (!done)
    {
        struct timeval tv = tv_page_flip_max_wait;
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(drm_fd, &fds);

        /*
         * Wait $page_flip_max_wait_usec for a page flip event. If we get a
         * page flip event, page_flip_handler(), called through drmHandleEvent(),
         * will update the pending_page_flips map. If we don't get the expected
         * page flip event within that time, or we can't read from the DRM fd, act
         * as if the page flip has occurred anyway.
         *
         * The rationale is that if we don't get a page flip event "soon" after
         * scheduling the page flip, something is severely broken at the driver
         * level. In that case, acting as if the page flip has occurred will not
         * cause any worse harm anyway (perhaps some tearing), and will allow us to
         * continue processing instead of just hanging.
         */
        auto ret = select(drm_fd + 1, &fds, nullptr, nullptr, &tv);

        {
            std::unique_lock<std::mutex> lock{pf_mutex};

            if (ret > 0)
            {
                drmHandleEvent(drm_fd, &evctx);
            }
            else
            {
                pending_page_flips.erase(crtc_id);
                listener->report_page_flip_timeout();
            }

            done = page_flip_is_done(crtc_id);
            /* Give up loop control if we are done */
            if (done)
                worker_tid = invalid_tid;
        }

        /*
         * Wake up other (non-worker) threads, so they can check whether
         * their page-flip events have arrived, or whether they can become
         * the worker (see pf_cv.wait(lock) above).
         */
        pf_cv.notify_all();
    }
}

std::thread::id mgg::KMSPageFlipManager::debug_get_worker_tid()
{
    std::unique_lock<std::mutex> lock{pf_mutex};

    return worker_tid;
}

/* This method should be called with the 'pf_mutex' locked */
bool mgg::KMSPageFlipManager::page_flip_is_done(uint32_t crtc_id)
{
    return pending_page_flips.find(crtc_id) == pending_page_flips.end();
}
