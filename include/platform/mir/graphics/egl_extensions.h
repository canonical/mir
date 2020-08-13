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
 *
 * This file also contains a subset of EGL definitions taken from eglext.h.
 * These are:
 *
 * Copyright (c) 2013-2017 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */


#ifndef MIR_GRAPHICS_EGL_EXTENSIONS_H_
#define MIR_GRAPHICS_EGL_EXTENSIONS_H_

#include <experimental/optional>

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

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

#ifndef EGL_KHR_debug
#define EGL_KHR_debug 1
typedef void *EGLLabelKHR;
typedef void *EGLObjectKHR;
typedef void (EGLAPIENTRY  *EGLDEBUGPROCKHR)(EGLenum error,const char *command,EGLint messageType,EGLLabelKHR threadLabel,EGLLabelKHR objectLabel,const char* message);
#define EGL_OBJECT_THREAD_KHR             0x33B0
#define EGL_OBJECT_DISPLAY_KHR            0x33B1
#define EGL_OBJECT_CONTEXT_KHR            0x33B2
#define EGL_OBJECT_SURFACE_KHR            0x33B3
#define EGL_OBJECT_IMAGE_KHR              0x33B4
#define EGL_OBJECT_SYNC_KHR               0x33B5
#define EGL_OBJECT_STREAM_KHR             0x33B6
#define EGL_DEBUG_MSG_CRITICAL_KHR        0x33B9
#define EGL_DEBUG_MSG_ERROR_KHR           0x33BA
#define EGL_DEBUG_MSG_WARN_KHR            0x33BB
#define EGL_DEBUG_MSG_INFO_KHR            0x33BC
#define EGL_DEBUG_CALLBACK_KHR            0x33B8
typedef EGLint (EGLAPIENTRYP PFNEGLDEBUGMESSAGECONTROLKHRPROC) (EGLDEBUGPROCKHR callback, const EGLAttrib *attrib_list);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYDEBUGKHRPROC) (EGLint attribute, EGLAttrib *value);
typedef EGLint (EGLAPIENTRYP PFNEGLLABELOBJECTKHRPROC) (EGLDisplay display, EGLenum objectType, EGLObjectKHR object, EGLLabelKHR label);
#endif

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

    class DebugKHR
    {
    public:
        DebugKHR();

        static DebugKHR extension_or_null_object();
        static std::experimental::optional<DebugKHR> maybe_debug_khr();

        PFNEGLDEBUGMESSAGECONTROLKHRPROC const eglDebugMessageControlKHR;
        PFNEGLLABELOBJECTKHRPROC const eglLabelObjectKHR;
        PFNEGLQUERYDEBUGKHRPROC const eglQueryDebugKHR;

    private:
        // Private helper for null object constructor.
        DebugKHR(
            PFNEGLDEBUGMESSAGECONTROLKHRPROC control,
            PFNEGLLABELOBJECTKHRPROC label,
            PFNEGLQUERYDEBUGKHRPROC query);
    };
};

}
}

#endif /* MIR_GRAPHICS_EGL_EXTENSIONS_H_ */
