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

#ifndef MIR_GRAPHICS_MESA_REAL_KMS_OUTPUT_H_
#define MIR_GRAPHICS_MESA_REAL_KMS_OUTPUT_H_

#include "kms_output.h"
#include "drm_mode_resources.h"

#include <memory>
#include <mutex>

namespace mir
{
namespace graphics
{
namespace mesa
{

class PageFlipper;

class RealKMSOutput : public KMSOutput
{
public:
    RealKMSOutput(int drm_fd, uint32_t connector_id,
                  std::shared_ptr<PageFlipper> const& page_flipper);
    ~RealKMSOutput();

    void reset();
    void configure(geometry::Displacement fb_offset, size_t kms_mode_index);
    geometry::Size size() const;
    int max_refresh_rate() const;

    bool set_crtc(uint32_t fb_id);
    void clear_crtc();
    bool schedule_page_flip(uint32_t fb_id);
    void wait_for_page_flip();

    void set_cursor(gbm_bo* buffer);
    void move_cursor(geometry::Point destination);
    void clear_cursor();
    bool has_cursor() const;

    void set_power_mode(MirPowerMode mode);

private:
    bool ensure_crtc();
    void restore_saved_crtc();

    int const drm_fd;
    uint32_t const connector_id;
    std::shared_ptr<PageFlipper> const page_flipper;

    DRMModeConnectorUPtr connector;
    size_t mode_index;
    geometry::Displacement fb_offset;
    DRMModeCrtcUPtr current_crtc;
    drmModeCrtc saved_crtc;
    bool using_saved_crtc;
    bool has_cursor_;

    MirPowerMode power_mode;
    int dpms_enum_id;

    std::mutex power_mutex;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_REAL_KMS_OUTPUT_H_ */
