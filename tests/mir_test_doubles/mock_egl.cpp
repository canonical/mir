/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by:
 * Thomas Voss <thomas.voss@canonical.com>
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/test/doubles/mock_egl.h"
#include "mir/client/egl_native_surface.h"

#include <gtest/gtest.h>
#include <dlfcn.h>

namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{
mtd::MockEGL* global_mock_egl = NULL;
auto filter_egl(decltype(&dlopen) real_dlopen, char const* filename, int flags) -> std::optional<void*>
{
    if (strcmp(filename, "libEGL.so.1") == 0)
    {
        // We want to pull symbols out of our binary, not the real libEGL
        return {real_dlopen(nullptr, flags)};
    }
    return {};
}
}


EGLConfig configs[] =
{
    (void*)3,
    (void*)4,
    (void*)8,
    (void*)14
};
EGLint config_size = 4;

/* We prefix GL/EGL extensions with "extension_" so code under test has to get their function
   ptrs with eglGetProcAddress */
EGLImageKHR extension_eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target,
                                        EGLClientBuffer buffer, const EGLint *attrib_list);
EGLBoolean extension_eglDestroyImageKHR (EGLDisplay dpy, EGLImageKHR image);
void extension_glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image);
EGLSyncKHR extension_eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list);
EGLBoolean extension_eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync);
EGLint extension_eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout);
EGLBoolean extension_eglGetSyncValuesCHROMIUM(EGLDisplay dpy,
    EGLSurface surface, int64_t *ust, int64_t *msc, int64_t *sbc);
EGLBoolean extension_eglBindWaylandDisplayWL(
    EGLDisplay dpy,
    struct wl_display *display);
EGLBoolean extension_eglUnbindWaylandDisplayWL(
    EGLDisplay dpy,
    struct wl_display *display);
EGLBoolean extension_eglQueryWaylandBufferWL(
    EGLDisplay dpy,
    struct wl_resource *buffer,
    EGLint attribute, EGLint *value);
EGLDisplay extension_eglGetPlatformDisplayEXT(
    EGLenum platform,
    void *native_display,
    const EGLint *attrib_list);
EGLSurface extension_eglCreatePlatformWindowSurfaceEXT(
    EGLDisplay dpy,
    EGLConfig config,
    void *native_window,
    const EGLint *attrib_list);


/* EGL{Surface,Display,Config,Context} are all opaque types, so we can put whatever
   we want in them for testing */
mtd::MockEGL::MockEGL()
    : fake_egl_display((EGLDisplay) 0x0530),
      fake_configs(configs),
      fake_configs_num(config_size),
      fake_egl_surface((EGLSurface) 0xa034),
      fake_egl_context((EGLContext) 0xbeef),
      fake_egl_image((EGLImageKHR) 0x1234),
      fake_visual_id(1),
      libegl_interposer{mtf::add_dlopen_filter(&filter_egl)}
{
    using namespace testing;
    assert(global_mock_egl == NULL && "Only one mock object per process is allowed");

    global_mock_egl = this;

    ON_CALL(*this, eglQueryString(_, EGL_VERSION))
        .WillByDefault(Return("1.4 Stub Mir EGL"));
    ON_CALL(*this, eglGetDisplay(_))
    .WillByDefault(Return(fake_egl_display));
    ON_CALL(*this, eglGetPlatformDisplayEXT(_,_,_))
        .WillByDefault(Return(fake_egl_display));
    ON_CALL(*this, eglInitialize(_,_,_))
    .WillByDefault(DoAll(
                       SetArgPointee<1>(1),
                       SetArgPointee<2>(4),
                       Return(EGL_TRUE)));
    ON_CALL(*this, eglBindApi(EGL_OPENGL_ES_API))
    .WillByDefault(Return(EGL_TRUE));

    ON_CALL(*this, eglGetConfigs(_,NULL, 0, _))
    .WillByDefault(DoAll(
                       SetArgPointee<3>(config_size),
                       Return(EGL_TRUE)));

    ON_CALL(*this, eglGetConfigAttrib(_, _, EGL_NATIVE_VISUAL_ID, _))
    .WillByDefault(DoAll(
                       SetArgPointee<3>(fake_visual_id),
                       Return(EGL_TRUE)));

    ON_CALL(*this, eglChooseConfig(_,_,_,_,_))
    .WillByDefault(Invoke(
        [&] (EGLDisplay, const EGLint *, EGLConfig *configs,
             EGLint config_size, EGLint *num_config) -> EGLBoolean
        {
            if (configs != nullptr)
            {
                EGLint const min_size = std::min(config_size, fake_configs_num);
                std::copy(fake_configs, fake_configs + min_size, configs);
                *num_config = min_size;
            }
            else
            {
                *num_config = fake_configs_num;
            }
            return EGL_TRUE;
        }));

    ON_CALL(*this, eglCreateWindowSurface(_,_,_,_))
        .WillByDefault(Return(fake_egl_surface));
    ON_CALL(*this, eglCreatePlatformWindowSurfaceEXT(_,_,_,_))
        .WillByDefault(Return(fake_egl_surface));

    ON_CALL(*this, eglCreatePbufferSurface(_,_,_))
    .WillByDefault(Return(fake_egl_surface));

    ON_CALL(*this, eglCreateContext(_,_,_,_))
    .WillByDefault(Return(fake_egl_context));

    ON_CALL(*this, eglMakeCurrent(_,_,_,_))
    .WillByDefault(Invoke(
        [this] (EGLDisplay, EGLSurface, EGLSurface, EGLContext context)
        {
            std::lock_guard<decltype(current_contexts_mutex)> lock{current_contexts_mutex};
            current_contexts[std::this_thread::get_id()] = context;
            return EGL_TRUE;
        }));

    ON_CALL(*this, eglGetCurrentContext())
    .WillByDefault(Invoke([this]
        {
            std::lock_guard<decltype(current_contexts_mutex)> lock{current_contexts_mutex};
            return current_contexts[std::this_thread::get_id()];
        }));

    ON_CALL(*this, eglSwapBuffers(_,_))
        .WillByDefault(Return(EGL_TRUE));                              

    ON_CALL(*this, eglGetCurrentDisplay())
    .WillByDefault(Return(fake_egl_display));

    ON_CALL(*this, eglCreateImageKHR(_,_,_,_,_))
    .WillByDefault(Return(fake_egl_image));

    typedef mtd::MockEGL::generic_function_pointer_t func_ptr_t;
    ON_CALL(*this, eglGetProcAddress(StrEq("eglCreateImageKHR")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(extension_eglCreateImageKHR)));
    ON_CALL(*this, eglGetProcAddress(StrEq("eglDestroyImageKHR")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(extension_eglDestroyImageKHR)));
    ON_CALL(*this, eglGetProcAddress(StrEq("glEGLImageTargetTexture2DOES")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(extension_glEGLImageTargetTexture2DOES)));
    ON_CALL(*this, eglGetProcAddress(StrEq("eglCreateSyncKHR")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(extension_eglCreateSyncKHR)));
    ON_CALL(*this, eglGetProcAddress(StrEq("eglDestroySyncKHR")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(extension_eglDestroySyncKHR)));
    ON_CALL(*this, eglGetProcAddress(StrEq("eglClientWaitSyncKHR")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(extension_eglClientWaitSyncKHR)));
    ON_CALL(*this, eglGetProcAddress(StrEq("eglGetSyncValuesCHROMIUM")))
        .WillByDefault(Return(
            reinterpret_cast<func_ptr_t>(extension_eglGetSyncValuesCHROMIUM)
            ));
    ON_CALL(*this, eglGetProcAddress(StrEq("eglQueryWaylandBufferWL")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(&extension_eglQueryWaylandBufferWL)));
    ON_CALL(*this, eglGetProcAddress(StrEq("eglBindWaylandDisplayWL")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(&extension_eglBindWaylandDisplayWL)));
    ON_CALL(*this, eglGetProcAddress(StrEq("eglUnbindWaylandDisplayWL")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(&extension_eglUnbindWaylandDisplayWL)));
    ON_CALL(*this, eglGetProcAddress(StrEq("eglGetPlatformDisplayEXT")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(&extension_eglGetPlatformDisplayEXT)));
    ON_CALL(*this, eglGetProcAddress(StrEq("eglCreatePlatformWindowSurfaceEXT")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(&extension_eglCreatePlatformWindowSurfaceEXT)));
}

void mtd::MockEGL::provide_egl_extensions()
{
    using namespace testing;

    const char* egl_exts =
        "EGL_KHR_image "
        "EGL_KHR_image_base "
        "EGL_KHR_image_pixmap "
        "EGL_EXT_image_dma_buf_import "
        "EGL_WL_bind_wayland_display "
        "EGL_EXT_platform_base";
    ON_CALL(*this, eglQueryString(_,EGL_EXTENSIONS))
        .WillByDefault(Return(egl_exts));
}

void mtd::MockEGL::provide_stub_platform_buffer_swapping()
{
    using namespace ::testing;
    // TODO: Comment
    ON_CALL(*this, eglCreateWindowSurface(_,_,_,_))
        .WillByDefault(Invoke(
            [&] (EGLDisplay,EGLConfig,AnyNativeType nw, EGLint const*) -> EGLSurface
            {
                return reinterpret_cast<EGLSurface>(nw);
            }));

    ON_CALL(*this, eglSwapBuffers(_,_))
        .WillByDefault(Invoke(
            [&](EGLDisplay,EGLSurface surface) -> EGLBoolean
            {
                auto mir_surf = reinterpret_cast<mir::client::EGLNativeSurface*>(surface);
                mir_surf->swap_buffers_sync();
                return true;
            }));
}

mtd::MockEGL::~MockEGL()
{
    global_mock_egl = NULL;
}

#define CHECK_GLOBAL_MOCK(rettype)         \
    if (!global_mock_egl)                  \
    {                                      \
        using namespace ::testing;         \
        ADD_FAILURE_AT(__FILE__,__LINE__); \
        rettype type = (rettype) 0;        \
        return type;                       \
    }

#define CHECK_GLOBAL_VOID_MOCK()            \
    if (!global_mock_egl)                   \
    {                                       \
        using namespace ::testing;          \
        ADD_FAILURE_AT(__FILE__,__LINE__);  \
        return;                             \
    }

#undef eglGetError
extern "C"
{
EGLint eglGetError (void);
}
EGLint eglGetError (void)
{
    CHECK_GLOBAL_MOCK(EGLint)
    return global_mock_egl->epoxy_eglGetError();
}

#undef eglGetDisplay
extern "C"
{
EGLDisplay eglGetDisplay (NativeDisplayType display);
}
EGLDisplay eglGetDisplay (NativeDisplayType display)
{
    CHECK_GLOBAL_MOCK(EGLDisplay);
    return global_mock_egl->epoxy_eglGetDisplay(reinterpret_cast<mtd::MockEGL::AnyNativeType>(display));
}

#undef eglInitialize
extern "C"
{
EGLBoolean eglInitialize (EGLDisplay dpy, EGLint *major, EGLint *minor);
}
EGLBoolean eglInitialize (EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglInitialize(dpy, major, minor);
}

#undef eglTerminate
extern "C"
{
EGLBoolean eglTerminate (EGLDisplay dpy);
}
EGLBoolean eglTerminate (EGLDisplay dpy)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglTerminate(dpy);
}

#undef eglQueryString
extern "C"
{
const char * eglQueryString (EGLDisplay dpy, EGLint name);
}
const char * eglQueryString (EGLDisplay dpy, EGLint name)
{
    CHECK_GLOBAL_MOCK(const char *)
    return global_mock_egl->epoxy_eglQueryString(dpy, name);
}

#undef eglBindAPI
extern "C"
{
EGLBoolean eglBindAPI (EGLenum api);
}
EGLBoolean eglBindAPI (EGLenum api)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->eglBindApi(api);
}

#undef eglGetProcAddress
extern "C"
{
mtd::MockEGL::generic_function_pointer_t eglGetProcAddress (const char *name);
}
mtd::MockEGL::generic_function_pointer_t eglGetProcAddress (const char *name)
{
    CHECK_GLOBAL_MOCK(mtd::MockEGL::generic_function_pointer_t)
    return global_mock_egl->epoxy_eglGetProcAddress(name);
}

#undef eglGetConfigs
extern "C"
{
EGLBoolean eglGetConfigs (EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);
}
EGLBoolean eglGetConfigs (EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglGetConfigs(dpy, configs, config_size, num_config);
}

#undef eglChooseConfig
extern "C"
{
EGLBoolean eglChooseConfig (EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
}
EGLBoolean eglChooseConfig (EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglChooseConfig(dpy, attrib_list, configs, config_size, num_config);
}

#undef eglGetConfigAttrib
extern "C"
{
EGLBoolean eglGetConfigAttrib (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);
}
EGLBoolean eglGetConfigAttrib (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglGetConfigAttrib(dpy, config, attribute, value);
}

#undef eglCreateWindowSurface
extern "C"
{
EGLSurface eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list);
}
EGLSurface eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list)
{
    CHECK_GLOBAL_MOCK(EGLSurface)
    return global_mock_egl->epoxy_eglCreateWindowSurface(dpy, config, reinterpret_cast<mtd::MockEGL::AnyNativeType>(window), attrib_list);
}

#undef eglCreatePixmapSurface
extern "C"
{
EGLSurface eglCreatePixmapSurface (EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list);
}
EGLSurface eglCreatePixmapSurface (EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list)
{
    CHECK_GLOBAL_MOCK(EGLSurface)
    return global_mock_egl->epoxy_eglCreatePixmapSurface(dpy, config, reinterpret_cast<mtd::MockEGL::AnyNativeType>(pixmap), attrib_list);
}

#undef eglCreatePbufferSurface
extern "C"
{
EGLSurface eglCreatePbufferSurface (EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
}
EGLSurface eglCreatePbufferSurface (EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
    CHECK_GLOBAL_MOCK(EGLSurface)
    return global_mock_egl->epoxy_eglCreatePbufferSurface(dpy, config, attrib_list);
}

#undef eglDestroySurface
extern "C"
{
EGLBoolean eglDestroySurface (EGLDisplay dpy, EGLSurface surface);
}
EGLBoolean eglDestroySurface (EGLDisplay dpy, EGLSurface surface)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglDestroySurface(dpy, surface);
}

#undef eglQuerySurface
extern "C"
{
EGLBoolean eglQuerySurface (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value);
}
EGLBoolean eglQuerySurface (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglQuerySurface(dpy, surface, attribute, value);
}

/* EGL 1.1 render-to-texture APIs */
#undef eglSurfaceAttrib
extern "C"
{
EGLBoolean eglSurfaceAttrib (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value);
}
EGLBoolean eglSurfaceAttrib (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglSurfaceAttrib(dpy, surface, attribute, value);
}

#undef eglBindTexImage
extern "C"
{
EGLBoolean eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
}
EGLBoolean eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglBindTexImage(dpy, surface, buffer);
}

#undef eglReleaseTexImage
extern "C"
{
EGLBoolean eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
}
EGLBoolean eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglReleaseTexImage(dpy, surface, buffer);
}

/* EGL 1.1 swap control API */
#undef eglSwapInterval
extern "C"
{
EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval);
}
EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglSwapInterval(dpy, interval);
}

#undef eglCreateContext
extern "C"
{
EGLContext eglCreateContext (EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list);
}
EGLContext eglCreateContext (EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list)
{
    CHECK_GLOBAL_MOCK(EGLContext)
    return global_mock_egl->epoxy_eglCreateContext(dpy, config, share_list, attrib_list);
}

#undef eglDestroyContext
extern "C"
{
EGLBoolean eglDestroyContext (EGLDisplay dpy, EGLContext ctx);
}
EGLBoolean eglDestroyContext (EGLDisplay dpy, EGLContext ctx)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglDestroyContext(dpy, ctx);
}

#undef eglMakeCurrent
extern "C"
{
EGLBoolean eglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
}
EGLBoolean eglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglMakeCurrent(dpy, draw, read, ctx);
}

#undef eglGetCurrentContext
extern "C"
{
EGLContext eglGetCurrentContext (void);
}
EGLContext eglGetCurrentContext (void)
{
    CHECK_GLOBAL_MOCK(EGLContext)
    return global_mock_egl->epoxy_eglGetCurrentContext();
}

#undef eglGetCurrentSurface
extern "C"
{
EGLSurface eglGetCurrentSurface (EGLint readdraw);
}
EGLSurface eglGetCurrentSurface (EGLint readdraw)
{
    CHECK_GLOBAL_MOCK(EGLSurface)
    return global_mock_egl->epoxy_eglGetCurrentSurface(readdraw);
}

#undef eglGetCurrentDisplay
extern "C"
{
EGLDisplay eglGetCurrentDisplay (void);
}
EGLDisplay eglGetCurrentDisplay (void)
{
    CHECK_GLOBAL_MOCK(EGLDisplay)
    return global_mock_egl->epoxy_eglGetCurrentDisplay();
}

#undef eglQueryContext
extern "C"
{
EGLBoolean eglQueryContext (EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value);
}
EGLBoolean eglQueryContext (EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglQueryContext(dpy, ctx, attribute, value);
}

#undef eglWaitGL
extern "C"
{
EGLBoolean eglWaitGL (void);
}
EGLBoolean eglWaitGL (void)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglWaitGL();
}

#undef eglWaitNative
extern "C"
{
EGLBoolean eglWaitNative (EGLint engine);
}
EGLBoolean eglWaitNative (EGLint engine)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglWaitNative(engine);
}

#undef eglSwapBuffers
extern "C"
{
EGLBoolean eglSwapBuffers (EGLDisplay dpy, EGLSurface draw);
}
EGLBoolean eglSwapBuffers (EGLDisplay dpy, EGLSurface draw)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglSwapBuffers(dpy, draw);
}

#undef eglCopyBuffers
extern "C"
{
EGLBoolean eglCopyBuffers (EGLDisplay dpy, EGLSurface surface, NativePixmapType target);
}
EGLBoolean eglCopyBuffers (EGLDisplay dpy, EGLSurface surface, NativePixmapType target)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->epoxy_eglCopyBuffers(dpy, surface, reinterpret_cast<mtd::MockEGL::AnyNativeType>(target));
}

/* extensions */
EGLImageKHR extension_eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list)
{
    CHECK_GLOBAL_MOCK(EGLImageKHR)
    return global_mock_egl->eglCreateImageKHR(dpy, ctx, target, buffer, attrib_list);
}

EGLBoolean extension_eglDestroyImageKHR (EGLDisplay dpy, EGLImageKHR image)
{
    CHECK_GLOBAL_MOCK(EGLBoolean)
    return global_mock_egl->eglDestroyImageKHR(dpy, image);
}

void extension_glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_egl->glEGLImageTargetTexture2DOES(target, image);
}

EGLSyncKHR extension_eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
    CHECK_GLOBAL_MOCK(EGLSyncKHR);
    return global_mock_egl->eglCreateSyncKHR(dpy, type, attrib_list);
}

EGLBoolean extension_eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync)
{
    CHECK_GLOBAL_MOCK(EGLBoolean);
    return global_mock_egl->eglDestroySyncKHR(dpy, sync);
}

EGLint extension_eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout)
{
    CHECK_GLOBAL_MOCK(EGLint);
    return global_mock_egl->eglClientWaitSyncKHR(dpy, sync, flags, timeout);
}

EGLBoolean extension_eglGetSyncValuesCHROMIUM(EGLDisplay dpy,
              EGLSurface surface, int64_t *ust, int64_t *msc, int64_t *sbc)
{
    CHECK_GLOBAL_MOCK(EGLBoolean);
    return global_mock_egl->eglGetSyncValuesCHROMIUM(dpy, surface,
                                                     ust, msc, sbc);
}

EGLBoolean extension_eglBindWaylandDisplayWL(
    EGLDisplay dpy,
    struct wl_display *display)
{
    CHECK_GLOBAL_MOCK(EGLBoolean);
    return global_mock_egl->eglBindWaylandDisplayWL(dpy, display);
}

EGLBoolean extension_eglUnbindWaylandDisplayWL(
    EGLDisplay dpy,
    struct wl_display *display)
{
    CHECK_GLOBAL_MOCK(EGLBoolean);
    return global_mock_egl->eglUnbindWaylandDisplayWL(dpy, display);
}

EGLBoolean extension_eglQueryWaylandBufferWL(
    EGLDisplay dpy,
    struct wl_resource* buffer,
    EGLint attribute, EGLint* value)
{
    CHECK_GLOBAL_MOCK(EGLBoolean);
    return global_mock_egl->eglQueryWaylandBufferWL(
        dpy, buffer, attribute, value);
}


EGLDisplay extension_eglGetPlatformDisplayEXT(
    EGLenum platform,
    void *native_display,
    const EGLint *attrib_list)
{
    CHECK_GLOBAL_MOCK(EGLDisplay);
    return global_mock_egl->eglGetPlatformDisplayEXT(
        platform,
        native_display,
        attrib_list);
}

EGLSurface extension_eglCreatePlatformWindowSurfaceEXT(
    EGLDisplay dpy,
    EGLConfig config,
    void *native_window,
    const EGLint *attrib_list)
{
    CHECK_GLOBAL_MOCK(EGLSurface);
    return global_mock_egl->eglCreatePlatformWindowSurfaceEXT(
        dpy,
        config,
        native_window,
        attrib_list);
}
