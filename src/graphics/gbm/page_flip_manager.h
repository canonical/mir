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

#ifndef MIR_GRAPHICS_GBM_PAGE_FLIP_MANAGER_H_
#define MIR_GRAPHICS_GBM_PAGE_FLIP_MANAGER_H_

#include <cstdint>

namespace mir
{
namespace graphics
{
namespace gbm
{

class PageFlipManager
{
public:
    virtual ~PageFlipManager() {}

    virtual bool schedule_page_flip(uint32_t crtc_id, uint32_t fb_id) = 0;
    virtual void wait_for_page_flip(uint32_t crtc_id) = 0;

protected:
    PageFlipManager() = default;
    PageFlipManager(PageFlipManager const&) = delete;
    PageFlipManager& operator=(PageFlipManager const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_PAGE_FLIP_MANAGER_H_ */
