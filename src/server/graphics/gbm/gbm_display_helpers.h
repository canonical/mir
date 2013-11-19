/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_GBM_GBM_DISPLAY_HELPERS_H_
#define MIR_GRAPHICS_GBM_GBM_DISPLAY_HELPERS_H_

#include "drm_mode_resources.h"
#include "udev_wrapper.h"

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
namespace gbm
{

typedef std::unique_ptr<gbm_surface,std::function<void(gbm_surface*)>> GBMSurfaceUPtr;

namespace helpers
{

class DRMHelper
{
public:
    DRMHelper() : fd{-1} {}
    ~DRMHelper();

    DRMHelper(const DRMHelper &) = delete;
    DRMHelper& operator=(const DRMHelper&) = delete;

    void setup(std::shared_ptr<UdevContext> const& udev);
    int get_authenticated_fd();
    void auth_magic(drm_magic_t magic) const;

    void drop_master() const;
    void set_master() const;

    int fd;

private:
    // TODO: This herustic is temporary; should be replaced with
    // handling >1 DRM device.
    int is_appropriate_device(std::shared_ptr<UdevContext> const& udev, UdevDevice const& dev);

    int count_connections(int fd);

    int open_drm_device(std::shared_ptr<UdevContext> const& udev);
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

class EGLHelper
{
public:
    EGLHelper()
        : egl_display{EGL_NO_DISPLAY}, egl_config{0},
          egl_context{EGL_NO_CONTEXT}, egl_surface{EGL_NO_SURFACE},
          should_terminate_egl{false} {}

    ~EGLHelper() noexcept;

    EGLHelper(const EGLHelper&) = delete;
    EGLHelper& operator=(const EGLHelper&) = delete;

    void setup(GBMHelper const& gbm);
    void setup(GBMHelper const& gbm, EGLContext shared_context);
    void setup(GBMHelper const& gbm, gbm_surface* surface_gbm,
               EGLContext shared_context);

    bool swap_buffers();
    bool make_current();
    bool release_current();

    EGLContext context() { return egl_context; }

    void report_egl_configuration(std::function<void(EGLDisplay, EGLConfig)>);
private:
    void setup_internal(GBMHelper const& gbm, bool initialize);

    EGLDisplay egl_display;
    EGLConfig egl_config;
    EGLContext egl_context;
    EGLSurface egl_surface;
    bool should_terminate_egl;
};

}
}
}
}
#endif /* MIR_GRAPHICS_GBM_GBM_DISPLAY_HELPERS_H_ */
