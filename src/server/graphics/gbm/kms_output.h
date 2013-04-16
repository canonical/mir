/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_GBM_KMS_OUTPUT_H_
#define MIR_GRAPHICS_GBM_KMS_OUTPUT_H_

#include "mir/geometry/size.h"
#include "drm_mode_resources.h"

#include <memory>

namespace mir
{
namespace graphics
{
namespace gbm
{

class PageFlipper;

class KMSOutput
{
public:
    KMSOutput(int drm_fd, uint32_t connector_id,
              std::shared_ptr<PageFlipper> const& page_flipper);
    ~KMSOutput();

    void reset();

    geometry::Size size() const;
    bool set_crtc(uint32_t fb_id);
    bool schedule_page_flip(uint32_t fb_id);
    void wait_for_page_flip();

private:
    KMSOutput(const KMSOutput&) = delete;
    KMSOutput& operator=(const KMSOutput&) = delete;

    bool ensure_crtc();
    void restore_saved_crtc();

    int const drm_fd;
    uint32_t const connector_id;
    std::shared_ptr<PageFlipper> const page_flipper;

    DRMModeConnectorUPtr connector;
    size_t mode_index;
    DRMModeCrtcUPtr current_crtc;
    drmModeCrtc saved_crtc;
    bool using_saved_crtc;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_KMS_OUTPUT_H_ */
