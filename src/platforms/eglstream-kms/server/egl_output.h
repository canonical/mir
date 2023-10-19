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

#ifndef MIR_GRAPHICS_EGLSTREAM_KMS_OUTPUT_H_
#define MIR_GRAPHICS_EGLSTREAM_KMS_OUTPUT_H_

#include "kms-utils/drm_mode_resources.h"
#include "kms_framebuffer.h"
#include "mir/geometry/size.h"
#include "mir/geometry/point.h"
#include "mir/geometry/displacement.h"
#include "mir/graphics/display_configuration.h"
#include "mir_toolkit/common.h"

#include <epoxy/egl.h>

namespace mir
{
namespace graphics
{
namespace eglstream
{

namespace kms
{
class EGLOutput : public DisplayConfigurationOutput
{
public:
    EGLOutput(int drm_fd, EGLDisplay dpy, EGLOutputPortEXT connector);
    ~EGLOutput();

    void reset();
    void configure(size_t kms_mode_index);
    geometry::Size size() const;
    int max_refresh_rate() const;

    EGLOutputLayerEXT output_layer() const;
    uint32_t crtc_id() const;
    bool queue_atomic_flip(FBHandle const& fb, void const* drm_event_userdata);
    void clear_crtc();

    void set_power_mode(MirPowerMode mode);
    void set_flags_for_next_flip(uint32_t flags);

private:
    void restore_saved_crtc();
    int atomic_commit(uint64_t fb, void const* drm_event_userdata, uint32_t flags);

    int const drm_fd;

    EGLDisplay display;
    EGLOutputPortEXT port;
    EGLOutputLayerEXT layer;

    graphics::kms::DRMModeConnectorUPtr connector;

    mir::graphics::kms::DRMModeCrtcUPtr current_crtc;
    size_t mode_index;
    drmModeCrtc saved_crtc;
    bool using_saved_crtc;

    uint32_t plane_id;
    std::unique_ptr<graphics::kms::ObjectProperties> plane_props;
    uint32_t _crtc_id;

    uint32_t mode_id;

    int dpms_enum_id;
    uint32_t flags_for_next_flip;
};

}
}
}
}

#endif /* MIR_GRAPHICS_EGLSTREAM_KMS_KMS_OUTPUT_H_ */
