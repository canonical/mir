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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "display_helpers.h"
#include "drm_close_threadsafe.h"

#include "mir/graphics/gl_config.h"
#include "mir/graphics/egl_error.h"
#include "mir/udev/wrapper.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <xf86drm.h>
#include <fcntl.h>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace mgmh = mir::graphics::mesa::helpers;

/*************
 * DRMHelper *
 *************/

void mgmh::DRMHelper::setup(std::shared_ptr<mir::udev::Context> const& udev)
{
    fd = open_drm_device(udev);

    if (fd < 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to open DRM device\n"));
}

mir::Fd mgmh::DRMHelper::authenticated_fd()
{
    /* We must have our own device fd first, so that it has become the DRM master */
    if (fd < 0)
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Tried to get authenticated DRM fd before setting up the DRM master"));

    if (node_to_use == DRMNodeToUse::render)
        return mir::Fd{IntOwnedFd{dup(fd)}};

    char* busid = drmGetBusid(fd);
    if (!busid)
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to get BusID of DRM device")) << boost::errinfo_errno(errno));
    int auth_fd = drmOpen(NULL, busid);
    drmFreeBusid(busid);

    if (auth_fd < 0)
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to open DRM device for authenticated fd"));

    if (fcntl(auth_fd, F_SETFD, fcntl(auth_fd, F_GETFD) | FD_CLOEXEC) == -1)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to set FD_CLOEXEC for authenticated drm fd")));
    }

    drm_magic_t magic;
    int ret = -1;
    if ((ret = drmGetMagic(auth_fd, &magic)) < 0)
    {
        close(auth_fd);
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to get DRM device magic cookie")) << boost::errinfo_errno(-ret));
    }

    if ((ret = drmAuthMagic(fd, magic)) < 0)
    {
        close(auth_fd);
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to authenticate DRM device magic cookie")) << boost::errinfo_errno(-ret));
    }

    //TODO: remove IntOwnedFd, its how the code works now though
    return mir::Fd{IntOwnedFd{auth_fd}};
}

void mgmh::DRMHelper::auth_magic(drm_magic_t magic)
{
    /* We must have our own device fd first, so that it has become the DRM master */
    if (fd < 0)
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Tried to authenticate magic cookie before setting up the DRM master"));
    }

    int ret = drmAuthMagic(fd, magic);

    if (ret < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to authenticate DRM device magic cookie")) << boost::errinfo_errno(-ret));
    }
}

void mgmh::DRMHelper::drop_master() const
{
    /* We must have our own device fd first, so that it has become the DRM master */
    if (fd < 0)
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Tried to drop DRM master without a DRM device"));
    }

    int ret = drmDropMaster(fd);

    if (ret < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to drop DRM master"))
                    << boost::errinfo_errno(errno));
    }
}

void mgmh::DRMHelper::set_master() const
{
    /* We must have our own device fd first, so that it has become the DRM master */
    if (fd < 0)
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Tried to set DRM master without a DRM device"));
    }

    int ret = drmSetMaster(fd);

    if (ret < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to set DRM master"))
                    << boost::errinfo_errno(errno));
    }
}

int mgmh::DRMHelper::is_appropriate_device(std::shared_ptr<mir::udev::Context> const& udev, mir::udev::Device const& drm_device)
{
    mir::udev::Enumerator children(udev);
    children.match_parent(drm_device);

    char const* devtype = drm_device.devtype();
    if (!devtype || strcmp(devtype, "drm_minor"))
        return EINVAL;

    children.scan_devices();
    for (auto& device : children)
    {
        // For some reason udev regards the device as a parent of itself
        // If there are any other children, they should be outputs.
        if (device != drm_device)
            return 0;
    }

    return ENOMEDIUM;
}

int mgmh::DRMHelper::count_connections(int fd)
{
    DRMModeResources resources{fd};

    int n_connected = 0;
    resources.for_each_connector([&](DRMModeConnectorUPtr connector)
    {
        if (connector->connection == DRM_MODE_CONNECTED)
            n_connected++;
    });

    return n_connected;
}

int mgmh::DRMHelper::open_drm_device(std::shared_ptr<mir::udev::Context> const& udev)
{
    int tmp_fd = -1;
    int error = ENODEV; //Default error is "there are no DRM devices"

    mir::udev::Enumerator devices(udev);
    devices.match_subsystem("drm");
    if (node_to_use == DRMNodeToUse::render)
        devices.match_sysname("renderD[0-9]*");
    else
        devices.match_sysname("card[0-9]*");

    devices.scan_devices();

    for(auto& device : devices)
    {
        if ((node_to_use == DRMNodeToUse::card) && (error = is_appropriate_device(udev, device)))
            continue;

        // If directly opening the DRM device is good enough for X it's good enough for us!
        tmp_fd = open(device.devnode(), O_RDWR | O_CLOEXEC);
        if (tmp_fd < 0)
        {
            error = errno;
            continue;
        }

        if (node_to_use == DRMNodeToUse::card)
        {
            // Check that the drm device is usable by setting the interface version we use (1.4)
            drmSetVersion sv;
            sv.drm_di_major = 1;
            sv.drm_di_minor = 4;
            sv.drm_dd_major = -1;     /* Don't care */
            sv.drm_dd_minor = -1;     /* Don't care */

            if ((error = -drmSetInterfaceVersion(tmp_fd, &sv)))
            {
                close(tmp_fd);
                tmp_fd = -1;
                continue;
            }

            // Stop if this device has connections to display on
            if (count_connections(tmp_fd) > 0)
                break;

            close(tmp_fd);
            tmp_fd = -1;
        }
        else
            break;
    }

    if (tmp_fd < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Error opening DRM device")) << boost::errinfo_errno(error));
    }

    return tmp_fd;
}

mgmh::DRMHelper::~DRMHelper()
{
    if (fd >= 0)
        mgm::drm_close_threadsafe(fd);
}

/*************
 * GBMHelper *
 *************/

void mgmh::GBMHelper::setup(const DRMHelper& drm)
{
    device = gbm_create_device(drm.fd);
    if (!device)
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to create GBM device"));
}

void mgmh::GBMHelper::setup(int drm_fd)
{
    device = gbm_create_device(drm_fd);
    if(!device)
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to create GBM device"));
}

mgm::GBMSurfaceUPtr mgmh::GBMHelper::create_scanout_surface(uint32_t width, uint32_t height)
{
    auto surface_raw = gbm_surface_create(device, width, height,
                                          GBM_BO_FORMAT_XRGB8888,
                                          GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

    auto gbm_surface_deleter = [](gbm_surface *p) { if (p) gbm_surface_destroy(p); };
    GBMSurfaceUPtr surface{surface_raw, gbm_surface_deleter};

    if (!surface)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create GBM scanout surface"));

    return surface;
}

mgmh::GBMHelper::~GBMHelper()
{
    if (device)
        gbm_device_destroy(device);
}

/*************
 * EGLHelper *
 *************/

mgmh::EGLHelper::EGLHelper(GLConfig const& gl_config)
    : depth_buffer_bits{gl_config.depth_buffer_bits()},
      stencil_buffer_bits{gl_config.stencil_buffer_bits()},
      egl_display{EGL_NO_DISPLAY}, egl_config{0},
      egl_context{EGL_NO_CONTEXT}, egl_surface{EGL_NO_SURFACE},
      should_terminate_egl{false}
{
}

void mgmh::EGLHelper::setup(GBMHelper const& gbm)
{
    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    setup_internal(gbm, true);

    egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
}

void mgmh::EGLHelper::setup(GBMHelper const& gbm, EGLContext shared_context)
{
    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    setup_internal(gbm, false);

    egl_context = eglCreateContext(egl_display, egl_config, shared_context, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
}

void mgmh::EGLHelper::setup(GBMHelper const& gbm, gbm_surface* surface_gbm,
                            EGLContext shared_context)
{
    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    setup_internal(gbm, false);

    egl_surface = eglCreateWindowSurface(egl_display, egl_config, surface_gbm, nullptr);
    if(egl_surface == EGL_NO_SURFACE)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL window surface"));

    egl_context = eglCreateContext(egl_display, egl_config, shared_context, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
}

mgmh::EGLHelper::~EGLHelper() noexcept
{
    if (egl_display != EGL_NO_DISPLAY) {
        if (egl_context != EGL_NO_CONTEXT)
        {
            if (eglGetCurrentContext() == egl_context)
                eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            eglDestroyContext(egl_display, egl_context);
        }
        if (egl_surface != EGL_NO_SURFACE)
            eglDestroySurface(egl_display, egl_surface);
        if (should_terminate_egl)
            eglTerminate(egl_display);
    }
}

bool mgmh::EGLHelper::swap_buffers()
{
    auto ret = eglSwapBuffers(egl_display, egl_surface);
    return (ret == EGL_TRUE);
}

bool mgmh::EGLHelper::make_current() const
{
    auto ret = eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
    return (ret == EGL_TRUE);
}

bool mgmh::EGLHelper::release_current() const
{
    auto ret = eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    return (ret == EGL_TRUE);
}

void mgmh::EGLHelper::setup_internal(GBMHelper const& gbm, bool initialize)
{
    EGLint const config_attr[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 5,
        EGL_GREEN_SIZE, 5,
        EGL_BLUE_SIZE, 5,
        EGL_ALPHA_SIZE, 0,
        EGL_DEPTH_SIZE, depth_buffer_bits,
        EGL_STENCIL_SIZE, stencil_buffer_bits,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    static const EGLint required_egl_version_major = 1;
    static const EGLint required_egl_version_minor = 4;

    EGLint num_egl_configs;

    egl_display = eglGetDisplay(static_cast<EGLNativeDisplayType>(gbm.device));
    if (egl_display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to get EGL display"));

    if (initialize)
    {
        EGLint major, minor;

        if (eglInitialize(egl_display, &major, &minor) == EGL_FALSE)
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to initialize EGL display"));

        if ((major != required_egl_version_major) || (minor != required_egl_version_minor))
        {
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(std::runtime_error("Incompatible EGL version")));
            // TODO: Insert egl version major and minor into exception
        }

        should_terminate_egl = true;
    }

    eglBindAPI(EGL_OPENGL_ES_API);

    if (eglChooseConfig(egl_display, config_attr, &egl_config, 1, &num_egl_configs) == EGL_FALSE ||
        num_egl_configs != 1)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to choose ARGB EGL config"));
    }
}

void mgmh::EGLHelper::report_egl_configuration(std::function<void(EGLDisplay, EGLConfig)> f)
{
    f(egl_display, egl_config);
}
