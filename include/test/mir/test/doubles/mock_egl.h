/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_EGL_H_
#define MIR_TEST_DOUBLES_MOCK_EGL_H_

#include <mir/synchronised.h>
#include <gmock/gmock.h>

#include <mutex>
#include <thread>
#include <unordered_map>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
//for GL extensions
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

struct wl_display;
struct wl_resource;

namespace mir
{
namespace test
{
namespace doubles
{

MATCHER_P(AttrMatches, val, std::string("matches"))
{
    auto i = 0;
    while ((val[i] != EGL_NONE) && (arg[i] != EGL_NONE))
    {
        if (val[i] != arg[i])
            return false;
        i++;
    }

    if ((val[i] == EGL_NONE) && (arg[i] == EGL_NONE))
    {
        return true;
    }
    return false;
}

MATCHER_P2(EGLConfigContainsAttrib, attrib, value, "")
{
    bool attrib_position = true;
    bool attrib_found = false;

    // arg is const, but we need to mutate it in the loop (needed for gtest 1.8.1+)
    auto arg_mut = arg;

    while (!attrib_position || *arg_mut != EGL_NONE)
    {
        if (attrib_position && *arg_mut == attrib)
        {
            attrib_found = true;
        }
        else if (!attrib_position)
        {
            if (attrib_found && *arg_mut == value)
            {
                return true;
            }

            attrib_found = false;
        }
        attrib_position = !attrib_position;
        ++arg_mut;
    }

    size_t end{0};
    for (; arg[end] != EGL_NONE; ++end);

    *result_listener << "attribute list is " << testing::PrintToString(std::vector<EGLint>(arg, arg + end));

    return false;
}

class MockEGL
{
public:
    MockEGL();
    virtual ~MockEGL();

    void expect_nested_egl_usage();
    void provide_egl_extensions();

    // Provide a functional version of eglSwapBuffers on stubbed platforms
    // When enabled, if an instance of mir::client::EGLNativeSurface is passed to
    // eglCreateWindowSurface, then the returned EGLSurface can be used with
    // eglSwapBuffers to invoke EGLNativeSurface::swap_buffers_sync
    void provide_stub_platform_buffer_swapping();

    typedef void (*generic_function_pointer_t)(void);
    typedef void* AnyNativeType;

    MOCK_METHOD(EGLDisplay, eglGetDisplay, (AnyNativeType));
    MOCK_METHOD(EGLBoolean, eglInitialize, (EGLDisplay,EGLint*,EGLint*));
    MOCK_METHOD(EGLBoolean, eglTerminate, (EGLDisplay));
    MOCK_METHOD(const char*, eglQueryString,(EGLDisplay, EGLint));
    MOCK_METHOD(EGLBoolean, eglBindApi, (EGLenum));
    MOCK_METHOD(generic_function_pointer_t, eglGetProcAddress, (const char*));

    MOCK_METHOD(EGLDisplay, eglGetPlatformDisplayEXT, (EGLenum, AnyNativeType, EGLint const*));

    // Config management
    MOCK_METHOD(EGLBoolean, eglGetConfigs, (EGLDisplay,EGLConfig*,EGLint,EGLint*));
    MOCK_METHOD(EGLBoolean, eglChooseConfig, (EGLDisplay, const EGLint*,EGLConfig*,EGLint,EGLint*));
    MOCK_METHOD(EGLBoolean, eglGetConfigAttrib, (EGLDisplay,EGLConfig,EGLint,EGLint*));

    // Surface management
    MOCK_METHOD(EGLSurface, eglCreatePlatformWindowSurfaceEXT, (EGLDisplay,EGLConfig,AnyNativeType, EGLint const*));
    MOCK_METHOD(EGLSurface, eglCreateWindowSurface, (EGLDisplay,EGLConfig,AnyNativeType,const EGLint*));
    MOCK_METHOD(EGLSurface, eglCreatePixmapSurface, (EGLDisplay,EGLConfig,AnyNativeType,const EGLint*));
    MOCK_METHOD(EGLSurface, eglCreatePbufferSurface, (EGLDisplay,EGLConfig,const EGLint*));
    MOCK_METHOD(EGLBoolean, eglDestroySurface, (EGLDisplay,EGLSurface));
    MOCK_METHOD(EGLBoolean, eglQuerySurface, (EGLDisplay,EGLSurface,EGLint,EGLint*));

    // EGL 1.1 render-to-texture APIs
    MOCK_METHOD(EGLBoolean, eglSurfaceAttrib, (EGLDisplay,EGLSurface,EGLint,EGLint));
    MOCK_METHOD(EGLBoolean, eglBindTexImage, (EGLDisplay,EGLSurface,EGLint));
    MOCK_METHOD(EGLBoolean, eglReleaseTexImage, (EGLDisplay,EGLSurface,EGLint));

    // EGL 1.1 swap control API
    MOCK_METHOD(EGLBoolean, eglSwapInterval, (EGLDisplay,EGLint));

    MOCK_METHOD(EGLContext, eglCreateContext, (EGLDisplay,EGLConfig,EGLContext,const EGLint*));
    MOCK_METHOD(EGLBoolean, eglDestroyContext, (EGLDisplay,EGLContext));
    MOCK_METHOD(EGLBoolean, eglMakeCurrent, (EGLDisplay,EGLSurface,EGLSurface,EGLContext));
    MOCK_METHOD(EGLContext, eglGetCurrentContext,());
    MOCK_METHOD(EGLSurface, eglGetCurrentSurface, (EGLint));
    MOCK_METHOD(EGLDisplay, eglGetCurrentDisplay, ());
    MOCK_METHOD(EGLBoolean, eglQueryContext, (EGLDisplay,EGLContext,EGLint,EGLint*));

    MOCK_METHOD(EGLBoolean, eglWaitGL, ());
    MOCK_METHOD(EGLBoolean, eglWaitNative, (EGLint));
    MOCK_METHOD(EGLBoolean, eglSwapBuffers, (EGLDisplay,EGLSurface));
    MOCK_METHOD(EGLBoolean, eglCopyBuffers, (EGLDisplay,EGLSurface,AnyNativeType));

    MOCK_METHOD(EGLint, eglGetError, ());

    MOCK_METHOD(EGLImageKHR, eglCreateImageKHR, (EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLint*));
    MOCK_METHOD(EGLBoolean, eglDestroyImageKHR,(EGLDisplay, EGLImageKHR));
    MOCK_METHOD(void, glEGLImageTargetTexture2DOES, (GLenum, GLeglImageOES));

    MOCK_METHOD(EGLSyncKHR, eglCreateSyncKHR, (EGLDisplay, EGLenum, EGLint const*));
    MOCK_METHOD(EGLBoolean, eglDestroySyncKHR, (EGLDisplay, EGLSyncKHR));
    MOCK_METHOD(EGLint, eglClientWaitSyncKHR, (EGLDisplay, EGLSyncKHR, EGLint, EGLTimeKHR));

    MOCK_METHOD(EGLBoolean, eglGetSyncValuesCHROMIUM, (EGLDisplay, EGLSurface, int64_t*, int64_t*, int64_t*));

    MOCK_METHOD(EGLBoolean, eglBindWaylandDisplayWL, (EGLDisplay, struct wl_display*));
    MOCK_METHOD(EGLBoolean, eglUnbindWaylandDisplayWL, (EGLDisplay, struct wl_display*));
    MOCK_METHOD(EGLBoolean, eglQueryWaylandBufferWL, (EGLDisplay, struct wl_resource*, EGLint, EGLint*));

    EGLDisplay const fake_egl_display;
    EGLConfig const* const fake_configs;
    EGLint const fake_configs_num;
    EGLSurface const fake_egl_surface;
    EGLContext const fake_egl_context;
    EGLImageKHR const fake_egl_image;
    int const fake_visual_id;
    Synchronised<std::unordered_map<std::thread::id,EGLContext>> current_contexts;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_EGL_H_ */
