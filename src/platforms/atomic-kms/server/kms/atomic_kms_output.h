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

#ifndef MIR_GRAPHICS_GBM_ATOMIC_KMS_OUTPUT_H_
#define MIR_GRAPHICS_GBM_ATOMIC_KMS_OUTPUT_H_

#include "kms_output.h"
#include "kms-utils/drm_mode_resources.h"
#include "mir/fd.h"

#include <memory>
#include <future>

namespace mir
{
namespace graphics
{
namespace kms
{
class DRMEventHandler;
}
namespace atomic
{

class PageFlipper;

class AtomicKMSOutput : public KMSOutput
{
public:
    AtomicKMSOutput(
        mir::Fd drm_master,
        kms::DRMModeConnectorUPtr connector,
        std::shared_ptr<kms::DRMEventHandler> event_handler);
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
    void wait_for_page_flip() override;

    bool set_cursor(gbm_bo* buffer) override;
    void move_cursor(geometry::Point destination) override;
    bool clear_cursor() override;
    bool has_cursor() const override;

    void set_power_mode(MirPowerMode mode) override;
    void set_gamma(GammaCurves const& gamma) override;

    void refresh_hardware_state() override;
    void update_from_hardware_state(DisplayConfigurationOutput& output) const override;

    int drm_fd() const override;

    bool has_cursor_plane() const;

    using CursorFbPtr = std::unique_ptr<uint32_t, std::function<void(uint32_t*)>>;
    struct CursorState {
        bool enabled{false};
        CursorFbPtr fb_id;
        uint32_t width, height;
        int crtc_x, crtc_y;
    };

private:
    bool ensure_crtc();
    void restore_saved_crtc();

    CursorFbPtr cursor_gbm_bo_to_drm_fb_id(gbm_bo* gbm_bo);

    mir::Fd const drm_fd_;
    std::shared_ptr<kms::DRMEventHandler> const event_handler;

    std::future<void> pending_page_flip;

    class PropertyBlob;

    kms::DRMModeConnectorUPtr connector;
    size_t mode_index;
    geometry::Displacement fb_offset;
    kms::DRMModeCrtcUPtr current_crtc;
    kms::DRMModePlaneUPtr current_primary_plane;
    kms::DRMModePlaneUPtr current_cursor_plane;
    std::unique_ptr<PropertyBlob> mode;
    std::unique_ptr<kms::ObjectProperties> crtc_props;
    std::unique_ptr<kms::ObjectProperties> primary_plane_props;
    std::unique_ptr<kms::ObjectProperties> cursor_plane_props;
    std::unique_ptr<kms::ObjectProperties> connector_props;
    drmModeCrtc saved_crtc;
    bool using_saved_crtc;

    CursorState cursor_state;
    std::mutex cursor_mutex;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_ATOMIC_KMS_OUTPUT_H_ */
