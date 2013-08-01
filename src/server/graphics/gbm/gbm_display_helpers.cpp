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

#include "gbm_display_helpers.h"
#include "drm_close_threadsafe.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <xf86drm.h>
#include <libudev.h>
#include <fcntl.h>

namespace mgg = mir::graphics::gbm;
namespace mggh = mir::graphics::gbm::helpers;

/**************
 * UdevHelper *
 **************/

mggh::UdevHelper::UdevHelper()
{
    ctx = udev_new();

    if (!ctx)
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to create udev context"));
}

mggh::UdevHelper::~UdevHelper() noexcept
{
    udev_unref(ctx);
}

/*************
 * DRMHelper *
 *************/

void mggh::DRMHelper::setup(UdevHelper const& udev)
{
    fd = open_drm_device(udev);

    if (fd < 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to open DRM device\n"));
}

int mggh::DRMHelper::get_authenticated_fd()
{
    /* We must have our own device fd first, so that it has become the DRM master */
    if (fd < 0)
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Tried to get authenticated DRM fd before setting up the DRM master"));

    char* busid = drmGetBusid(fd);
    if (!busid)
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to get BusID of DRM device")) << boost::errinfo_errno(errno));
    int auth_fd = drmOpen(NULL, busid);
    free(busid);

    if (auth_fd < 0)
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to open DRM device for authenticated fd"));

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

    return auth_fd;
}

void mggh::DRMHelper::auth_magic(drm_magic_t magic) const
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

void mggh::DRMHelper::drop_master() const
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
                    << boost::errinfo_errno(-ret));
    }
}

void mggh::DRMHelper::set_master() const
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
                    << boost::errinfo_errno(-ret));
    }
}

int mggh::DRMHelper::is_appropriate_device(UdevHelper const& udev, udev_device* drm_device)
{
    udev_enumerate* children = udev_enumerate_new(udev.ctx);
    udev_enumerate_add_match_parent(children, drm_device);

    char const* devtype = udev_device_get_devtype(drm_device);
    if (!devtype || strcmp(devtype, "drm_minor"))
        return EINVAL;

    if (!udev_enumerate_scan_devices(children))
    {
        udev_list_entry *devices, *device;
        devices = udev_enumerate_get_list_entry(children);
        udev_list_entry_foreach(device, devices) 
        {
            char const* sys_path = udev_list_entry_get_name(device);

            // For some reason udev regards the device as a parent of itself
            // If there are any other children, they should be outputs.
            if (strcmp(sys_path, udev_device_get_syspath(drm_device)))
            {
                udev_enumerate_unref(children);
                return 0;
            }
        }
    }
    udev_enumerate_unref(children);

    return ENOMEDIUM;
}

int mggh::DRMHelper::count_connections(int fd)
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

int mggh::DRMHelper::open_drm_device(UdevHelper const& udev)
{    
    int tmp_fd = -1;
    int error;

    // TODO: Wrap this up in a nice class
    udev_enumerate* enumerator = udev_enumerate_new(udev.ctx);
    udev_enumerate_add_match_subsystem(enumerator, "drm");
    udev_enumerate_add_match_sysname(enumerator, "card[0-9]");

    udev_list_entry *devices, *device;

    if ((error = udev_enumerate_scan_devices(enumerator)))
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to enumerate udev devices")) << boost::errinfo_errno(-error));

    devices = udev_enumerate_get_list_entry(enumerator);
    udev_list_entry_foreach(device, devices)
    {
        char const* sys_path = udev_list_entry_get_name(device);
        udev_device* dev = udev_device_new_from_syspath(udev.ctx, sys_path);

        // Devices can disappear on us.
        if (!dev)
            continue;

        if ((error = is_appropriate_device(udev, dev)))
        {
            udev_device_unref(dev);
            continue;
        }

        char const* dev_path = udev_device_get_devnode(dev);

        // If directly opening the DRM device is good enough for X it's good enough for us!
        tmp_fd = open(dev_path, O_RDWR, O_CLOEXEC);
        if (tmp_fd < 0)
        {
            error = errno;
            udev_device_unref(dev);
            continue;
        }

        udev_device_unref(dev);

        // Check that the drm device is usable by setting the interface version we use (1.4)
        drmSetVersion sv;
        sv.drm_di_major = 1;
        sv.drm_di_minor = 4;
        sv.drm_dd_major = -1;     /* Don't care */
        sv.drm_dd_minor = -1;     /* Don't care */

        if ((error = drmSetInterfaceVersion(tmp_fd, &sv)))
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
    udev_enumerate_unref(enumerator);

    if (tmp_fd < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Error opening DRM device")) << boost::errinfo_errno(-error));
    }

    return tmp_fd;
}

mggh::DRMHelper::~DRMHelper()
{
    if (fd >= 0)
        mgg::drm_close_threadsafe(fd);
}

/*************
 * GBMHelper *
 *************/

void mggh::GBMHelper::setup(const DRMHelper& drm)
{
    device = gbm_create_device(drm.fd);
    if (!device)
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to create GBM device"));
}

mgg::GBMSurfaceUPtr mggh::GBMHelper::create_scanout_surface(uint32_t width, uint32_t height)
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

mggh::GBMHelper::~GBMHelper()
{
    if (device)
        gbm_device_destroy(device);
}

/*************
 * EGLHelper *
 *************/

void mggh::EGLHelper::setup(GBMHelper const& gbm)
{
    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    setup_internal(gbm, true);

    egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create EGL context"));
}

void mggh::EGLHelper::setup(GBMHelper const& gbm, EGLContext shared_context)
{
    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    setup_internal(gbm, false);

    egl_context = eglCreateContext(egl_display, egl_config, shared_context, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create EGL context"));
}

void mggh::EGLHelper::setup(GBMHelper const& gbm, gbm_surface* surface_gbm,
                            EGLContext shared_context)
{
    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    setup_internal(gbm, false);

    egl_surface = eglCreateWindowSurface(egl_display, egl_config, surface_gbm, nullptr);
    if(egl_surface == EGL_NO_SURFACE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create EGL window surface"));

    egl_context = eglCreateContext(egl_display, egl_config, shared_context, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create EGL context"));
}

mggh::EGLHelper::~EGLHelper() noexcept
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

bool mggh::EGLHelper::swap_buffers()
{
    auto ret = eglSwapBuffers(egl_display, egl_surface);
    return (ret == EGL_TRUE);
}

bool mggh::EGLHelper::make_current()
{
    auto ret = eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
    return (ret == EGL_TRUE);
}

bool mggh::EGLHelper::release_current()
{
    auto ret = eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    return (ret == EGL_TRUE);
}

void mggh::EGLHelper::setup_internal(GBMHelper const& gbm, bool initialize)
{
    static const EGLint config_attr[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    static const EGLint required_egl_version_major = 1;
    static const EGLint required_egl_version_minor = 4;

    EGLint num_egl_configs;

    egl_display = eglGetDisplay(static_cast<EGLNativeDisplayType>(gbm.device));
    if (egl_display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to get EGL display"));

    if (initialize)
    {
        EGLint major, minor;

        if (eglInitialize(egl_display, &major, &minor) == EGL_FALSE)
            BOOST_THROW_EXCEPTION(std::runtime_error("Failed to initialize EGL display"));

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
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to choose ARGB EGL config"));
    }
}

void mggh::EGLHelper::report_egl_configuration(std::function<void(EGLDisplay, EGLConfig)> f)
{
    f(egl_display, egl_config);
}
