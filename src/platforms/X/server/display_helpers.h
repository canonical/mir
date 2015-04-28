/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_GRAPHICS_X_DISPLAY_HELPERS_H_
#define MIR_GRAPHICS_X_DISPLAY_HELPERS_H_

#include "drm_mode_resources.h"
#include "drm_authentication.h"
#include "mir/udev/wrapper.h"

#include <cstddef>
#include <memory>

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wall"
#include <gbm.h>
#pragma GCC diagnostic pop

#include <EGL/egl.h>
#include <xf86drmMode.h>

namespace mir
{
namespace graphics
{
namespace X
{

typedef std::unique_ptr<gbm_surface,std::function<void(gbm_surface*)>> GBMSurfaceUPtr;

namespace helpers
{

class DRMHelper : public DRMAuthentication
{
public:
    DRMHelper() : fd{-1} {}
    ~DRMHelper();

    DRMHelper(const DRMHelper &) = delete;
    DRMHelper& operator=(const DRMHelper&) = delete;

    void setup(std::shared_ptr<mir::udev::Context> const& udev);
    mir::Fd authenticated_fd();
    void auth_magic(drm_magic_t magic);

//    void drop_master() const;
//    void set_master() const;

    int fd;

private:
    // TODO: This herustic is temporary; should be replaced with
    // handling >1 DRM device.
//    int is_appropriate_device(std::shared_ptr<mir::udev::Context> const& udev, mir::udev::Device const& dev);

//    int count_connections(int fd);

    int open_drm_device(std::shared_ptr<mir::udev::Context> const& udev);
};

class GBMHelper
{
public:
    GBMHelper() : device{0} {}
    ~GBMHelper();

    GBMHelper(const GBMHelper&) = delete;
    GBMHelper& operator=(const GBMHelper&) = delete;

    void setup(const DRMHelper& drm);
    void setup(int drm_fd);
    GBMSurfaceUPtr create_scanout_surface(uint32_t width, uint32_t height);

    gbm_device* device;
};

}
}
}
}
#endif /* MIR_GRAPHICS_X_DISPLAY_HELPERS_H_ */
