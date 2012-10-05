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

#include "mir/graphics/gbm/gbm_display_helpers.h"

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
        throw std::runtime_error("Failed to open DRM device\n");
}

int mggh::DRMHelper::get_authenticated_fd()
{
    /* We must have our own device fd first, so that it has become the DRM master */
    if (fd < 0)
        throw std::runtime_error("Tried to get authenticated DRM fd before setting up the DRM master\n");

    int auth_fd = open_drm_device();

    if (auth_fd < 0)
        throw std::runtime_error("Failed to open DRM device for authenticated fd\n");

    drm_magic_t magic;

    if (drmGetMagic(auth_fd, &magic))
    {
        close(auth_fd);
        throw std::runtime_error("Failed to get DRM device magic cookie\n");
    }

    if (drmAuthMagic(fd, magic))
    {
        close(auth_fd);
        throw std::runtime_error("Failed to authenticate DRM device magic cookie\n");
    }

    return auth_fd;
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
    int i;

    resources = drmModeGetResources(drm.fd);
    if (!resources)
        throw std::runtime_error("Couldn't get DRM resources\n");

    /* Find the first connected connector */
    for (i = 0; i < resources->count_connectors; i++)
    {
        connector = drmModeGetConnector(drm.fd, resources->connectors[i]);
        if (connector == NULL)
            continue;

        if (connector->connection == DRM_MODE_CONNECTED &&
            connector->count_modes > 0)
        {
            break;
        }

        drmModeFreeConnector(connector);
        connector = NULL;
    }

    if (connector == NULL)
        throw std::runtime_error("No active DRM connector found\n");

    /* Find the encoder connected to the selected connector */
    for (i = 0; i < resources->count_encoders; i++)
    {
        encoder = drmModeGetEncoder(drm.fd, resources->encoders[i]);

        if (encoder == NULL)
            continue;

        if (encoder->encoder_id == connector->encoder_id)
            break;

        drmModeFreeEncoder(encoder);
        encoder = NULL;
    }

    if (encoder == NULL)
        throw std::runtime_error("No connected DRM encoder found\n");

    mode = connector->modes[0];

    drm_fd = drm.fd;
    saved_crtc = drmModeGetCrtc(drm.fd, encoder->crtc_id);
}

mggh::KMSHelper::~KMSHelper()
{
    if (saved_crtc)
    {
        drmModeSetCrtc(drm_fd, saved_crtc->crtc_id, saved_crtc->buffer_id,
                       saved_crtc->x, saved_crtc->y,
                       &connector->connector_id, 1, &saved_crtc->mode);
        drmModeFreeCrtc(saved_crtc);
    }

    if (encoder)
        drmModeFreeEncoder(encoder);
    if (connector)
        drmModeFreeConnector(connector);
    if (resources)
        drmModeFreeResources(resources);
}

/*************
 * GBMHelper *
 *************/

void mggh::GBMHelper::setup(const DRMHelper& drm)
{
    device = gbm_create_device(drm.fd);
    if (!device)
        throw std::runtime_error("Failed to create GBM device");
}

void mggh::GBMHelper::create_scanout_surface(uint32_t width, uint32_t height)
{
    surface = gbm_surface_create(device, width, height,
                                 GBM_BO_FORMAT_XRGB8888,
                                 GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!surface)
        throw std::runtime_error("Failed to create GBM scanout surface");
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

    EGLint major, minor;
    EGLint num_configs;

    display = eglGetDisplay(static_cast<EGLNativeDisplayType>(gbm.device));
    if (display == EGL_NO_DISPLAY)
        throw std::runtime_error("Failed to get EGL display");

    if (eglInitialize(display, &major, &minor) == EGL_FALSE)
        throw std::runtime_error("Failed to initialize EGL display");

    if ((major != 1) || (minor != 4))
        throw std::runtime_error("Incompatible EGL version (!= 1.4) found");

    eglBindAPI(EGL_OPENGL_ES_API);

    if (!eglChooseConfig(display, config_attr, &config, 1, &num_configs) ||
        num_configs != 1)
    {
        throw std::runtime_error("Failed to choose ARGB EGL config");
    }

    surface = eglCreateWindowSurface(display, config, gbm.surface, NULL);
    if(surface == EGL_NO_SURFACE)
        throw std::runtime_error("Failed to create EGL window surface");

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attr);
    if (context == EGL_NO_CONTEXT)
        throw std::runtime_error("Failed to create EGL context");
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
