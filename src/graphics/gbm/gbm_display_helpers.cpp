/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <xf86drm.h>

namespace mggh=mir::graphics::gbm::helpers;

/*************
 * DRMHelper *
 *************/

void mggh::DRMHelper::setup()
{
    fd = open_drm_device();

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

    int auth_fd = open_drm_device();

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
                std::runtime_error("Failed to get DRM device magic cookie")) << boost::errinfo_errno(ret));
    }

    if ((ret = drmAuthMagic(fd, magic)) < 0)
    {
        close(auth_fd);
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to authenticate DRM device magic cookie")) << boost::errinfo_errno(ret));
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
                std::runtime_error("Failed to authenticate DRM device magic cookie")) << boost::errinfo_errno(ret));
    }
}

int mggh::DRMHelper::open_drm_device()
{
    static const char *drivers[] = {
        "i915", "radeon", "nouveau", 0
    };
    int tmp_fd = -1;

    const char** driver{drivers};

    while (tmp_fd < 0 && *driver)
    {
        tmp_fd = drmOpen(*driver, NULL);
        ++driver;
    }

    if (tmp_fd < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Problem opening DRM device")) << boost::errinfo_errno(-tmp_fd));
    }

    return tmp_fd;
}

mggh::DRMHelper::~DRMHelper()
{
    if (fd >= 0)
        drmClose(fd);
}

/*************
 * KMSHelper *
 *************/

void mggh::KMSHelper::setup(const DRMHelper& drm)
{
    DRMModeResources resources{drm.fd};

    /* Find the first connected connector */
    resources.for_each_connector([&](DRMModeConnectorUPtr con)
    {
        if (!connector &&
            con->connection == DRM_MODE_CONNECTED &&
            con->count_modes > 0)
        {
            connector = std::move(con);
        }
    });

    if (!connector)
        BOOST_THROW_EXCEPTION(std::runtime_error("No active DRM connector found\n"));

    encoder = std::move(resources.encoder(connector->encoder_id));
    if (encoder == NULL)
        BOOST_THROW_EXCEPTION(std::runtime_error("No connected DRM encoder found\n"));

    mode = connector->modes[0];

    drm_fd = drm.fd;
    saved_crtc = resources.crtc(encoder->crtc_id);
}

mggh::KMSHelper::~KMSHelper()
{
    if (saved_crtc)
    {
        drmModeSetCrtc(drm_fd, saved_crtc->crtc_id, saved_crtc->buffer_id,
                       saved_crtc->x, saved_crtc->y,
                       &connector->connector_id, 1, &saved_crtc->mode);
    }
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

void mggh::GBMHelper::create_scanout_surface(uint32_t width, uint32_t height)
{
    surface = gbm_surface_create(device, width, height,
                                 GBM_BO_FORMAT_XRGB8888,
                                 GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!surface)
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to create GBM scanout surface"));
}

mggh::GBMHelper::~GBMHelper()
{
    if (surface)
        gbm_surface_destroy(surface);
    if (device)
        gbm_device_destroy(device);
}

/*************
 * EGLHelper *
 *************/

void mggh::EGLHelper::setup(const GBMHelper& gbm)
{
    static const EGLint context_attr [] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    static const EGLint config_attr[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_DEPTH_SIZE, 1,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    static const EGLint required_egl_version_major = 1;
    static const EGLint required_egl_version_minor = 4;

    EGLint major, minor;
    EGLint num_configs;

    display = eglGetDisplay(static_cast<EGLNativeDisplayType>(gbm.device));
    if (display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to get EGL display"));

    if (eglInitialize(display, &major, &minor) == EGL_FALSE)
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to initialize EGL display"));

    if ((major != required_egl_version_major) || (minor != required_egl_version_minor))
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(std::runtime_error("Incompatible EGL version")));
        // TODO: Insert egl version major and minor into exception
    }
    eglBindAPI(EGL_OPENGL_ES_API);

    if (!eglChooseConfig(display, config_attr, &config, 1, &num_configs) ||
        num_configs != 1)
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to choose ARGB EGL config"));
    }

    surface = eglCreateWindowSurface(display, config, gbm.surface, NULL);
    if(surface == EGL_NO_SURFACE)
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to create EGL window surface"));

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attr);
    if (context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to create EGL context"));
}

mggh::EGLHelper::~EGLHelper()
{
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context != EGL_NO_CONTEXT)
            eglDestroyContext(display, context);
        if (surface != EGL_NO_SURFACE)
            eglDestroySurface(display, surface);
        eglTerminate(display);
    }
}
