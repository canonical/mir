/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_GRAPHICS_GBM_ATOMIC_KMS_ATOMIC_OUTPUT_H_
#define MIR_GRAPHICS_GBM_ATOMIC_KMS_ATOMIC_OUTPUT_H_

#include "kms_output.h"
#include "kms-utils/drm_mode_resources.h"
#include "mir/fd.h"
#include "mir/synchronised.h"

#include <memory>
#include <future>

namespace mir
{
namespace graphics
{
namespace atomic
{

class PageFlipper;

class AtomicKMSOutput : public KMSOutput
{
public:
    AtomicKMSOutput(
        mir::Fd drm_master,
        kms::DRMModeConnectorUPtr connector);
    ~AtomicKMSOutput();

    uint32_t id() const override;

    void reset() override;
    void configure(geometry::Displacement fb_offset, size_t kms_mode_index) override;
    geometry::Size size() const override;
    int max_refresh_rate() const override;

    bool set_crtc(FBHandle const& fb) override;
    bool has_crtc_mismatch() override;
    void clear_crtc() override;
    bool schedule_page_flip(FBHandle const& fb) override;

    void set_cursor(gbm_bo* buffer) override;
    void move_cursor(geometry::Point destination) override;
    bool clear_cursor() override;
    bool has_cursor() const override;

    void set_power_mode(MirPowerMode mode) override;
    void set_gamma(GammaCurves const& gamma) override;

    void refresh_hardware_state() override;
    void update_from_hardware_state(DisplayConfigurationOutput& output) const override;

    int drm_fd() const override;

private:
    class PropertyBlob;

    struct Configuration
    {
        kms::DRMModeConnectorUPtr connector;
        size_t mode_index;
        geometry::Displacement fb_offset;
        kms::DRMModeCrtcUPtr current_crtc;
        kms::DRMModePlaneUPtr current_plane;
        std::unique_ptr<PropertyBlob> mode;
        std::unique_ptr<kms::ObjectProperties> crtc_props;
        std::unique_ptr<kms::ObjectProperties> plane_props;
        std::unique_ptr<kms::ObjectProperties> connector_props;
    };

    bool ensure_crtc(Configuration& to_update);
    void restore_saved_crtc();

    mir::Fd const drm_fd_;

    std::future<void> pending_page_flip;

    mir::Synchronised<Configuration> configuration;
    drmModeCrtc saved_crtc;
    bool using_saved_crtc;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_ATOMIC_KMS_ATOMIC_OUTPUT_H_ */
