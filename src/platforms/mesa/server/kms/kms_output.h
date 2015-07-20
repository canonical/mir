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

#ifndef MIR_GRAPHICS_MESA_KMS_OUTPUT_H_
#define MIR_GRAPHICS_MESA_KMS_OUTPUT_H_

#include "mir/geometry/size.h"
#include "mir/geometry/point.h"
#include "mir/geometry/displacement.h"
#include "mir/graphics/display_configuration.h"
#include "mir_toolkit/common.h"

#include <gbm.h>

namespace mir
{
namespace graphics
{
namespace mesa
{

class KMSOutput
{
public:
    virtual ~KMSOutput() = default;

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

    virtual bool set_crtc(uint32_t fb_id) = 0;
    virtual void clear_crtc() = 0;
    virtual bool schedule_page_flip(uint32_t fb_id) = 0;
    virtual void wait_for_page_flip() = 0;

    virtual void set_cursor(gbm_bo* buffer) = 0;
    virtual void move_cursor(geometry::Point destination) = 0;
    virtual void clear_cursor() = 0;
    virtual bool has_cursor() const = 0;

    virtual void set_power_mode(MirPowerMode mode) = 0;

protected:
    KMSOutput() = default;
    KMSOutput(const KMSOutput&) = delete;
    KMSOutput& operator=(const KMSOutput&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_KMS_OUTPUT_H_ */
