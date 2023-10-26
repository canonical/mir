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
 *
 * This file also contains a subset of EGL definitions taken from eglext.h.
 * These are:
 *
 * Copyright (c) The Khronos Group Inc.
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

#include <optional>
#include <mutex>
#include <atomic>
#include <stdexcept>

#include <boost/throw_exception.hpp>

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

/*
 * Just enough polyfill for RPi's EGL headers...
 */
#ifndef EGL_KHR_stream
#define EGL_KHR_stream 1
typedef khronos_uint64_t EGLuint64KHR;
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

/*
 * More polyfill for the RPi's EGL headers...
 */
#ifndef EGL_EXT_image_dma_buf_import
#define EGL_EXT_image_dma_buf_import 1
#define EGL_LINUX_DMA_BUF_EXT             0x3270
#define EGL_LINUX_DRM_FOURCC_EXT          0x3271
#define EGL_DMA_BUF_PLANE0_FD_EXT         0x3272
#define EGL_DMA_BUF_PLANE0_OFFSET_EXT     0x3273
#define EGL_DMA_BUF_PLANE0_PITCH_EXT      0x3274
#define EGL_DMA_BUF_PLANE1_FD_EXT         0x3275
#define EGL_DMA_BUF_PLANE1_OFFSET_EXT     0x3276
#define EGL_DMA_BUF_PLANE1_PITCH_EXT      0x3277
#define EGL_DMA_BUF_PLANE2_FD_EXT         0x3278
#define EGL_DMA_BUF_PLANE2_OFFSET_EXT     0x3279
#define EGL_DMA_BUF_PLANE2_PITCH_EXT      0x327A
#define EGL_YUV_COLOR_SPACE_HINT_EXT      0x327B
#define EGL_SAMPLE_RANGE_HINT_EXT         0x327C
#define EGL_YUV_CHROMA_HORIZONTAL_SITING_HINT_EXT 0x327D
#define EGL_YUV_CHROMA_VERTICAL_SITING_HINT_EXT 0x327E
#define EGL_ITU_REC601_EXT                0x327F
#define EGL_ITU_REC709_EXT                0x3280
#define EGL_ITU_REC2020_EXT               0x3281
#define EGL_YUV_FULL_RANGE_EXT            0x3282
#define EGL_YUV_NARROW_RANGE_EXT          0x3283
#define EGL_YUV_CHROMA_SITING_0_EXT       0x3284
#define EGL_YUV_CHROMA_SITING_0_5_EXT     0x3285
#endif /* EGL_EXT_image_dma_buf_import */

#ifndef EGL_EXT_image_dma_buf_import_modifiers
#define EGL_EXT_image_dma_buf_import_modifiers 1
#define EGL_DMA_BUF_PLANE3_FD_EXT         0x3440
#define EGL_DMA_BUF_PLANE3_OFFSET_EXT     0x3441
#define EGL_DMA_BUF_PLANE3_PITCH_EXT      0x3442
#define EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT 0x3443
#define EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT 0x3444
#define EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT 0x3445
#define EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT 0x3446
#define EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT 0x3447
#define EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT 0x3448
#define EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT 0x3449
#define EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT 0x344A
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYDMABUFFORMATSEXTPROC) (EGLDisplay dpy, EGLint max_formats, EGLint *formats, EGLint *num_formats);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYDMABUFMODIFIERSEXTPROC) (EGLDisplay dpy, EGLint format, EGLint max_modifiers, EGLuint64KHR *modifiers, EGLBoolean *external_only, EGLint *num_modifiers);
#endif /* EGL_EXT_image_dma_buf_import_modifiers */

/*
 * Just enough polyfill for rawhide headers...
 */
extern "C" {
#ifndef EGL_WL_bind_wayland_display
#define EGL_WL_bind_wayland_display 1
#define PFNEGLBINDWAYLANDDISPLAYWL PFNEGLBINDWAYLANDDISPLAYWLPROC
#define PFNEGLUNBINDWAYLANDDISPLAYWL PFNEGLUNBINDWAYLANDDISPLAYWLPROC
#define PFNEGLQUERYWAYLANDBUFFERWL PFNEGLQUERYWAYLANDBUFFERWLPROC
struct wl_display;
struct wl_resource;
#define EGL_WAYLAND_BUFFER_WL             0x31D5
#define EGL_WAYLAND_PLANE_WL              0x31D6
#define EGL_TEXTURE_Y_U_V_WL              0x31D7
#define EGL_TEXTURE_Y_UV_WL               0x31D8
#define EGL_TEXTURE_Y_XUXV_WL             0x31D9
#define EGL_TEXTURE_EXTERNAL_WL           0x31DA
#define EGL_WAYLAND_Y_INVERTED_WL         0x31DB
typedef EGLBoolean (EGLAPIENTRYP PFNEGLBINDWAYLANDDISPLAYWLPROC) (EGLDisplay dpy, struct wl_display *display);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLUNBINDWAYLANDDISPLAYWLPROC) (EGLDisplay dpy, struct wl_display *display);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYWAYLANDBUFFERWLPROC) (EGLDisplay dpy, struct wl_resource *buffer, EGLint attribute, EGLint *value);
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglBindWaylandDisplayWL (EGLDisplay dpy, struct wl_display *display);
EGLAPI EGLBoolean EGLAPIENTRY eglUnbindWaylandDisplayWL (EGLDisplay dpy, struct wl_display *display);
EGLAPI EGLBoolean EGLAPIENTRY eglQueryWaylandBufferWL (EGLDisplay dpy, struct wl_resource *buffer, EGLint attribute, EGLint *value);
#endif
#endif /* EGL_WL_bind_wayland_display */
}

namespace mir
{
namespace graphics
{
auto has_egl_client_extension(char const* extension) -> bool;
auto has_egl_extension(EGLDisplay dpy, char const* extension) -> bool;

struct EGLExtensions
{
    template<typename Ext>
    struct LazyDisplayExtensions
    {
        auto operator()(EGLDisplay dpy) const -> Ext const&
        {
            if (!has_initialized)
            {
                std::lock_guard lock{mutex};
                if (!has_initialized)
                {
                    cached_display = dpy;
                    cached_ext.emplace(dpy);
                    has_initialized = true;
                }
            }

            if (dpy != cached_display)
            {
                BOOST_THROW_EXCEPTION(std::logic_error("Multiple EGL displays used with the same extension object"));
            }

            return cached_ext.value();
        }

    private:
        std::atomic<bool> mutable has_initialized{false};
        std::mutex mutable mutex;
        EGLDisplay mutable cached_display{EGL_NO_DISPLAY};
        std::optional<Ext> mutable cached_ext;
    };

    EGLExtensions();

    struct BaseExtensions
    {
        BaseExtensions(EGLDisplay dpy);

        PFNEGLCREATEIMAGEKHRPROC const eglCreateImageKHR;
        PFNEGLDESTROYIMAGEKHRPROC const eglDestroyImageKHR;
        PFNGLEGLIMAGETARGETTEXTURE2DOESPROC const glEGLImageTargetTexture2DOES;
    };
    LazyDisplayExtensions<BaseExtensions> const base;

    struct WaylandExtensions
    {
        WaylandExtensions(EGLDisplay dpy);

        PFNEGLBINDWAYLANDDISPLAYWL const eglBindWaylandDisplayWL;
        PFNEGLUNBINDWAYLANDDISPLAYWL const eglUnbindWaylandDisplayWL;
        PFNEGLQUERYWAYLANDBUFFERWL const eglQueryWaylandBufferWL;
    };
    LazyDisplayExtensions<WaylandExtensions> const wayland;

    struct NVStreamAttribExtensions
    {
        NVStreamAttribExtensions(EGLDisplay dpy);

        PFNEGLCREATESTREAMATTRIBNVPROC const eglCreateStreamAttribNV;
        PFNEGLSTREAMCONSUMERACQUIREATTRIBNVPROC const eglStreamConsumerAcquireAttribNV;
    };

    struct PlatformBaseEXT
    {
        PlatformBaseEXT();

        PFNEGLGETPLATFORMDISPLAYEXTPROC const eglGetPlatformDisplay;
        PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC const eglCreatePlatformWindowSurface;
    };
    std::optional<PlatformBaseEXT> const platform_base;

    class DebugKHR
    {
    public:
        DebugKHR();

        static DebugKHR extension_or_null_object();
        static std::optional<DebugKHR> maybe_debug_khr();

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

    struct EXTImageDmaBufImportModifiers
    {
        EXTImageDmaBufImportModifiers(EGLDisplay dpy);

        PFNEGLQUERYDMABUFFORMATSEXTPROC const eglQueryDmaBufFormatsExt;
        PFNEGLQUERYDMABUFMODIFIERSEXTPROC const eglQueryDmaBufModifiersExt;
    };

    struct MESADmaBufExport
    {
        MESADmaBufExport(EGLDisplay dpy);

        static auto extension_if_supported(EGLDisplay dpy) -> std::optional<MESADmaBufExport>;

        PFNEGLEXPORTDMABUFIMAGEMESAPROC const eglExportDMABUFImageMESA;
        PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC const eglExportDMABUFImageQueryMESA;
    };
};
}
}

#endif /* MIR_GRAPHICS_EGL_EXTENSIONS_H_ */
