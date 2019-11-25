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
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_GRAPHICS_EGL_EXTENSIONS_H_
#define MIR_GRAPHICS_EGL_EXTENSIONS_H_

#include <experimental/optional>

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include MIR_SERVER_GL_H
#include MIR_SERVER_GLEXT_H

// For Wayland extensions
#include <EGL/eglmesaext.h>

/*
 * Just enough polyfill for RPi's EGL headers...
 */
#ifndef EGL_KHR_stream
#define EGL_KHR_stream 1
typedef void* EGLStreamKHR;
#endif

#ifndef EGL_VERSION_1_5
#ifndef EGLAttribPolyfil
#define EGLAttribPolyfil
typedef intptr_t EGLAttrib;
#endif
#endif

#ifndef EGL_EXT_platform_base
#define EGL_EXT_platform_base 1
typedef EGLDisplay (EGLAPIENTRYP PFNEGLGETPLATFORMDISPLAYEXTPROC) (EGLenum platform, void *native_display, const EGLint *attrib_list);
typedef EGLSurface (EGLAPIENTRYP PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC) (EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
typedef EGLSurface (EGLAPIENTRYP PFNEGLCREATEPLATFORMPIXMAPSURFACEEXTPROC) (EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list);
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLDisplay EGLAPIENTRY eglGetPlatformDisplayEXT (EGLenum platform, void *native_display, const EGLint *attrib_list);
EGLAPI EGLSurface EGLAPIENTRY eglCreatePlatformWindowSurfaceEXT (EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
EGLAPI EGLSurface EGLAPIENTRY eglCreatePlatformPixmapSurfaceEXT (EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list);
#endif
#endif /* EGL_EXT_platform_base */

/*
 * FIXME: Remove both EGL_EXT_stream_acquire_mode and
 *        EGL_NV_output_drm_flip_event definitions below once both extensions
 *        get published by Khronos and incorportated into Khronos' header files
 */
#ifndef EGL_NV_stream_attrib
#define EGL_NV_stream_attrib 1
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLStreamKHR EGLAPIENTRY eglCreateStreamAttribNV(EGLDisplay dpy, const EGLAttrib *attrib_list);
EGLAPI EGLBoolean EGLAPIENTRY eglSetStreamAttribNV(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLAttrib value);
EGLAPI EGLBoolean EGLAPIENTRY eglQueryStreamAttribNV(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLAttrib *value);
EGLAPI EGLBoolean EGLAPIENTRY eglStreamConsumerAcquireAttribNV(EGLDisplay dpy, EGLStreamKHR stream, const EGLAttrib *attrib_list);
EGLAPI EGLBoolean EGLAPIENTRY eglStreamConsumerReleaseAttribNV(EGLDisplay dpy, EGLStreamKHR stream, const EGLAttrib *attrib_list);
#endif
typedef EGLStreamKHR (EGLAPIENTRYP PFNEGLCREATESTREAMATTRIBNVPROC) (EGLDisplay dpy, const EGLAttrib *attrib_list);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLSETSTREAMATTRIBNVPROC) (EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLAttrib value);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYSTREAMATTRIBNVPROC) (EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLAttrib *value);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLSTREAMCONSUMERACQUIREATTRIBNVPROC) (EGLDisplay dpy, EGLStreamKHR stream, const EGLAttrib *attrib_list);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLSTREAMCONSUMERRELEASEATTRIBNVPROC) (EGLDisplay dpy, EGLStreamKHR stream, const EGLAttrib *attrib_list);
#endif /* EGL_NV_stream_attrib */

#ifndef EGL_EXT_stream_acquire_mode
#define EGL_EXT_stream_acquire_mode 1
#define EGL_CONSUMER_AUTO_ACQUIRE_EXT         0x332B
typedef EGLBoolean (EGLAPIENTRYP PFNEGLSTREAMCONSUMERACQUIREATTRIBEXTPROC) (EGLDisplay dpy, EGLStreamKHR stream, const EGLAttrib *attrib_list);
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglStreamConsumerAcquireAttribEXT (EGLDisplay dpy, EGLStreamKHR stream, const EGLAttrib *attrib_list);
#endif
#endif /* EGL_EXT_stream_acquire_mode */

namespace mir
{
namespace graphics
{
struct EGLExtensions
{
    EGLExtensions();
    PFNEGLCREATEIMAGEKHRPROC const eglCreateImageKHR;
    PFNEGLDESTROYIMAGEKHRPROC const eglDestroyImageKHR;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC const glEGLImageTargetTexture2DOES;

    struct WaylandExtensions
    {
        WaylandExtensions();

        PFNEGLBINDWAYLANDDISPLAYWL const eglBindWaylandDisplayWL;
        PFNEGLQUERYWAYLANDBUFFERWL const eglQueryWaylandBufferWL;
    };
    std::experimental::optional<WaylandExtensions> const wayland;

    struct NVStreamAttribExtensions
    {
        NVStreamAttribExtensions();

        PFNEGLCREATESTREAMATTRIBNVPROC const eglCreateStreamAttribNV;
        PFNEGLSTREAMCONSUMERACQUIREATTRIBNVPROC const eglStreamConsumerAcquireAttribNV;
    };
    struct PlatformBaseEXT
    {
        PlatformBaseEXT();

        PFNEGLGETPLATFORMDISPLAYEXTPROC const eglGetPlatformDisplay;
        PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC const eglCreatePlatformWindowSurface;
    };
    std::experimental::optional<PlatformBaseEXT> const platform_base;
};

}
}

#endif /* MIR_GRAPHICS_EGL_EXTENSIONS_H_ */
