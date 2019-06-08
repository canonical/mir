/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_GRAPHICS_EGLSTREAM_KMS_OUTPUT_H_
#define MIR_GRAPHICS_EGLSTREAM_KMS_OUTPUT_H_

#include "kms-utils/drm_mode_resources.h"
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
    void clear_crtc();

    void set_power_mode(MirPowerMode mode);

private:
    void restore_saved_crtc();

    int const drm_fd;

    EGLDisplay display;
    EGLOutputPortEXT port;
    EGLOutputLayerEXT layer;

    graphics::kms::DRMModeConnectorUPtr connector;

    size_t mode_index;
    drmModeCrtc saved_crtc;
    bool using_saved_crtc;

    int dpms_enum_id;
};

}
}
}
}

#endif /* MIR_GRAPHICS_EGLSTREAM_KMS_KMS_OUTPUT_H_ */
