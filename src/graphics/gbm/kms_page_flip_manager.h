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

#ifndef MIR_GRAPHICS_GBM_KMS_PAGE_FLIP_MANAGER_H_
#define MIR_GRAPHICS_GBM_KMS_PAGE_FLIP_MANAGER_H_

#include "page_flip_manager.h"

#include <unordered_map>
#include <chrono>

namespace mir
{
namespace graphics
{
namespace gbm
{

struct PageFlipEventData
{
    std::unordered_map<uint32_t,PageFlipEventData>* pending;
    uint32_t crtc_id;
};

class KMSPageFlipManager : public PageFlipManager
{
public:
    KMSPageFlipManager(int drm_fd, std::chrono::microseconds max_wait);

    bool schedule_page_flip(uint32_t crtc_id, uint32_t fb_id);
    void wait_for_page_flip(uint32_t crtc_id);

private:
    int const drm_fd;
    long const page_flip_max_wait_usec;
    std::unordered_map<uint32_t,PageFlipEventData> pending_page_flips;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_KMS_PAGE_FLIP_MANAGER_H_ */
