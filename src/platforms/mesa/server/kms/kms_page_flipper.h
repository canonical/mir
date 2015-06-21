/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_KMS_PAGE_FLIPPER_H_
#define MIR_GRAPHICS_MESA_KMS_PAGE_FLIPPER_H_

#include "page_flipper.h"

#include <unordered_map>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <sys/time.h>

namespace mir
{
namespace graphics
{

class DisplayReport;

namespace mesa
{

class KMSPageFlipper;
struct PageFlipEventData
{
    uint32_t crtc_id;
    KMSPageFlipper* flipper;
};

class KMSPageFlipper : public PageFlipper
{
public:
    KMSPageFlipper(int drm_fd, std::shared_ptr<DisplayReport> const& report);

    bool schedule_flip(uint32_t crtc_id, uint32_t fb_id);
    void wait_for_flip(uint32_t crtc_id);

    std::thread::id debug_get_worker_tid();

    void notify_page_flip(uint32_t crtc_id); 
private:
    bool page_flip_is_done(uint32_t crtc_id);

    int const drm_fd;
    std::shared_ptr<DisplayReport> const report;
    std::unordered_map<uint32_t,PageFlipEventData> pending_page_flips;
    std::mutex pf_mutex;
    std::condition_variable pf_cv;
    std::thread::id worker_tid;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_KMS_PAGE_FLIPPER_H_ */
