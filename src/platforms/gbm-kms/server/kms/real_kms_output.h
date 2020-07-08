/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_REAL_KMS_OUTPUT_H_
#define MIR_GRAPHICS_MESA_REAL_KMS_OUTPUT_H_

#include "mir/graphics/atomic_frame.h"
#include "kms_output.h"
#include "kms-utils/drm_mode_resources.h"

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
    RealKMSOutput(
        int drm_fd,
        kms::DRMModeConnectorUPtr&& connector,
        std::shared_ptr<PageFlipper> const& page_flipper);
    ~RealKMSOutput();

    uint32_t id() const override;

    void reset() override;
    void configure(geometry::Displacement fb_offset, size_t kms_mode_index) override;
    geometry::Size size() const override;
    int max_refresh_rate() const override;

    bool set_crtc(FBHandle const& fb) override;
    void clear_crtc() override;
    bool schedule_page_flip(FBHandle const& fb) override;
    void wait_for_page_flip() override;

    bool set_cursor(gbm_bo* buffer) override;
    void move_cursor(geometry::Point destination) override;
    bool clear_cursor() override;
    bool has_cursor() const override;

    void set_power_mode(MirPowerMode mode) override;
    void set_gamma(GammaCurves const& gamma) override;

    Frame last_frame() const override;

    void refresh_hardware_state() override;
    void update_from_hardware_state(DisplayConfigurationOutput& output) const override;

    FBHandle* fb_for(gbm_bo* bo) const override;

    bool buffer_requires_migration(gbm_bo* bo) const override;
    int drm_fd() const override;
private:
    bool ensure_crtc();
    void restore_saved_crtc();

    int const drm_fd_;
    std::shared_ptr<PageFlipper> const page_flipper;

    kms::DRMModeConnectorUPtr connector;
    size_t mode_index;
    geometry::Displacement fb_offset;
    kms::DRMModeCrtcUPtr current_crtc;
    drmModeCrtc saved_crtc;
    bool using_saved_crtc;
    bool has_cursor_;

    MirPowerMode power_mode;
    int dpms_enum_id;

    std::mutex power_mutex;

    AtomicFrame last_frame_;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_REAL_KMS_OUTPUT_H_ */
