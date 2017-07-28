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

#ifndef MIR_GRAPHICS_MESA_KMS_OUTPUT_H_
#define MIR_GRAPHICS_MESA_KMS_OUTPUT_H_

#include "mir/geometry/size.h"
#include "mir/geometry/point.h"
#include "mir/geometry/displacement.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/frame.h"
#include "mir_toolkit/common.h"

#include "kms-utils/drm_mode_resources.h"

#include <gbm.h>

namespace mir
{
namespace graphics
{
class DisplayConfigurationOutput;

namespace mesa
{

class FBHandle;

class KMSOutput
{
public:
    virtual ~KMSOutput() = default;

    /*
     * I'm not sure that DRM guarantees ID uniqueness in the presence of hotplug/unplug;
     * this may want to be an opaque class Id + operator== in future.
     */
    virtual uint32_t id() const = 0;
    
    virtual void reset() = 0;
    virtual void configure(geometry::Displacement fb_offset, size_t kms_mode_index) = 0;
    virtual geometry::Size size() const = 0;

    /**
     * Approximate maximum refresh rate of this output to within 1Hz.
     * Typically the rate is fixed (e.g. 60Hz) but it may also be variable as
     * in Nvidia G-Sync/AMD FreeSync/VESA Adaptive Sync. So this function
     * returns the maximum rate to expect.
     */
    virtual int max_refresh_rate() const = 0;

    virtual bool set_crtc(FBHandle const& fb) = 0;
    virtual void clear_crtc() = 0;
    virtual bool schedule_page_flip(FBHandle const& fb) = 0;
    virtual void wait_for_page_flip() = 0;

    virtual bool set_cursor(gbm_bo* buffer) = 0;
    virtual void move_cursor(geometry::Point destination) = 0;
    virtual bool clear_cursor() = 0;
    virtual bool has_cursor() const = 0;

    virtual void set_power_mode(MirPowerMode mode) = 0;
    virtual void set_gamma(GammaCurves const& gamma) = 0;
    virtual Frame last_frame() const = 0;

    /**
     * Re-probe the hardware state of this connector.
     *
     * \throws std::system_error if the underlying DRM connector has disappeared.
     */
    virtual void refresh_hardware_state() = 0;
    /**
     * Translate and copy the cached hardware state into a Mir display configuration object.
     *
     * \param [out] to_update   The Mir display configuration object to update with new
     *                                  hardware state. Only hardware state (modes, dimensions, etc)
     *                                  is touched.
     */
    virtual void update_from_hardware_state(DisplayConfigurationOutput& to_update) const = 0;
    virtual FBHandle* fb_for(gbm_bo* bo) const = 0;

    /**
     * Check whether buffer need to be migrated to GPU-private memory for display.
     *
     * \param [in] bo   GBM buffer to test
     * \return  True if buffer must be migrated to display-private memory in order to be displayed.
     *          If this method returns true the caller should probably copy it to a new buffer before
     *          calling fb_for(buffer), as acquiring a FBHandle to the buffer will likely make it
     *          unusable for rendering on the original GPU.
     */
    virtual bool buffer_requires_migration(gbm_bo* bo) const = 0;

    virtual int drm_fd() const = 0;
protected:
    KMSOutput() = default;
    KMSOutput(const KMSOutput&) = delete;
    KMSOutput& operator=(const KMSOutput&) = delete;
};
}
}
}

#endif /* MIR_GRAPHICS_MESA_KMS_OUTPUT_H_ */
