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

#ifndef MIR_GRAPHICS_ATOMIC_DISPLAY_HELPERS_H_
#define MIR_GRAPHICS_ATOMIC_DISPLAY_HELPERS_H_

#include "mir/udev/wrapper.h"
#include "mir/fd.h"

#include <cstddef>
#include <memory>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wall"
#include <gbm.h>
#pragma GCC diagnostic pop

#include <xf86drmMode.h>

namespace mir
{
class ConsoleServices;
class Device;

namespace graphics
{
namespace atomic
{
class Quirks;

typedef std::unique_ptr<gbm_surface,std::function<void(gbm_surface*)>> GBMSurfaceUPtr;

namespace helpers
{

class DRMHelper
{
public:
    ~DRMHelper();

    DRMHelper(const DRMHelper &) = delete;
    DRMHelper& operator=(const DRMHelper&) = delete;

    static std::vector<std::shared_ptr<DRMHelper>> open_all_devices(
        std::shared_ptr<mir::udev::Context> const& udev,
        mir::ConsoleServices& console,
        Quirks const& quirks);

    static std::unique_ptr<DRMHelper> open_any_render_node(
        std::shared_ptr<mir::udev::Context> const& udev);

    mir::Fd fd;
private:
    std::unique_ptr<Device> const device_handle;

    explicit DRMHelper(mir::Fd&& fd, std::unique_ptr<mir::Device> device);
};

class GBMHelper
{
public:
    GBMHelper(mir::Fd const& drm_fd);
    ~GBMHelper();

    GBMHelper(const GBMHelper&) = delete;
    GBMHelper& operator=(const GBMHelper&) = delete;

    gbm_device* const device;
};

}
}
}
}
#endif /* MIR_GRAPHICS_ATOMIC_DISPLAY_HELPERS_H_ */
