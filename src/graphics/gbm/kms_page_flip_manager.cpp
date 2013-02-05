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

#include <xf86drm.h>
#include <xf86drmMode.h>

namespace mgg = mir::graphics::gbm;

namespace
{

void page_flip_handler(int /*fd*/, unsigned int /*frame*/,
                       unsigned int /*sec*/, unsigned int /*usec*/,
                       void* data)
{
    auto page_flip_data = static_cast<mgg::PageFlipEventData*>(data);
    page_flip_data->pending->erase(page_flip_data->crtc_id);
}

}

mgg::KMSPageFlipManager::KMSPageFlipManager(int drm_fd,
                                            std::chrono::microseconds max_wait)
    : drm_fd{drm_fd},
      page_flip_max_wait_usec{max_wait.count()},
      pending_page_flips()
{
}

bool mgg::KMSPageFlipManager::schedule_page_flip(uint32_t crtc_id, uint32_t fb_id)
{
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

    while (pending_page_flips.find(crtc_id) != pending_page_flips.end())
    {
        struct timeval tv{0, page_flip_max_wait_usec};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(drm_fd, &fds);

        /* Wait for an event from the DRM device */
        auto ret = select(drm_fd + 1, &fds, nullptr, nullptr, &tv);

        if (ret > 0)
            drmHandleEvent(drm_fd, &evctx);
        else
            pending_page_flips.erase(crtc_id);
    }
}
