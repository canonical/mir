/*
 * Copyright © 2012 Canonical Ltd.
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
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_EGL_H_
#define MIR_TEST_DOUBLES_MOCK_EGL_H_

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

    auto arg_mut = arg;

    while (!attrib_position || *arg_mut != EGL_NONE)
    {
        if (attrib_position && *arg_mut == attrib)
        {
            attrib_found = true;
        }
        else if (!attrib_position)
        {
            if (attrib_found && *arg == value)
            {
                return true;
            }

            attrib_found = false;
        }
        attrib_position = !attrib_position;
        ++arg_mut;
    }

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

    MOCK_METHOD1(eglGetDisplay, EGLDisplay(AnyNativeType));
    MOCK_METHOD3(eglInitialize, EGLBoolean(EGLDisplay,EGLint*,EGLint*));
    MOCK_METHOD1(eglTerminate, EGLBoolean(EGLDisplay));
    MOCK_METHOD2(eglQueryString,const char*(EGLDisplay, EGLint));
    MOCK_METHOD1(eglBindApi, EGLBoolean(EGLenum));
    MOCK_METHOD1(eglGetProcAddress,generic_function_pointer_t(const char*));

    // Config management
    MOCK_METHOD4(eglGetConfigs, EGLBoolean(EGLDisplay,EGLConfig*,EGLint,EGLint*));
    MOCK_METHOD5(eglChooseConfig, EGLBoolean(EGLDisplay, const EGLint*,EGLConfig*,EGLint,EGLint*));
    MOCK_METHOD4(eglGetConfigAttrib, EGLBoolean(EGLDisplay,EGLConfig,EGLint,EGLint*));

    // Surface management
    MOCK_METHOD4(eglCreateWindowSurface, EGLSurface(EGLDisplay,EGLConfig,AnyNativeType,const EGLint*));
    MOCK_METHOD4(eglCreatePixmapSurface, EGLSurface(EGLDisplay,EGLConfig,AnyNativeType,const EGLint*));
    MOCK_METHOD3(eglCreatePbufferSurface, EGLSurface(EGLDisplay,EGLConfig,const EGLint*));
    MOCK_METHOD2(eglDestroySurface, EGLBoolean(EGLDisplay,EGLSurface));
    MOCK_METHOD4(eglQuerySurface, EGLBoolean(EGLDisplay,EGLSurface,EGLint,EGLint*));

    // EGL 1.1 render-to-texture APIs
    MOCK_METHOD4(eglSurfaceAttrib, EGLBoolean(EGLDisplay,EGLSurface,EGLint,EGLint));
    MOCK_METHOD3(eglBindTexImage, EGLBoolean(EGLDisplay,EGLSurface,EGLint));
    MOCK_METHOD3(eglReleaseTexImage, EGLBoolean(EGLDisplay,EGLSurface,EGLint));

    // EGL 1.1 swap control API
    MOCK_METHOD2(eglSwapInterval, EGLBoolean(EGLDisplay,EGLint));

    MOCK_METHOD4(eglCreateContext, EGLContext(EGLDisplay,EGLConfig,EGLContext,const EGLint*));
    MOCK_METHOD2(eglDestroyContext, EGLBoolean(EGLDisplay,EGLContext));
    MOCK_METHOD4(eglMakeCurrent, EGLBoolean(EGLDisplay,EGLSurface,EGLSurface,EGLContext));
    MOCK_METHOD0(eglGetCurrentContext,EGLContext());
    MOCK_METHOD1(eglGetCurrentSurface,EGLSurface(EGLint));
    MOCK_METHOD0(eglGetCurrentDisplay, EGLDisplay());
    MOCK_METHOD4(eglQueryContext, EGLBoolean(EGLDisplay,EGLContext,EGLint,EGLint*));

    MOCK_METHOD0(eglWaitGL, EGLBoolean());
    MOCK_METHOD1(eglWaitNative, EGLBoolean(EGLint));
    MOCK_METHOD2(eglSwapBuffers, EGLBoolean(EGLDisplay,EGLSurface));
    MOCK_METHOD3(eglCopyBuffers, EGLBoolean(EGLDisplay,EGLSurface,AnyNativeType));

    MOCK_METHOD0(eglGetError, EGLint (void));

    MOCK_METHOD5(eglCreateImageKHR, EGLImageKHR(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLint*));
    MOCK_METHOD2(eglDestroyImageKHR,EGLBoolean(EGLDisplay, EGLImageKHR));
    MOCK_METHOD2(glEGLImageTargetTexture2DOES, void(GLenum, GLeglImageOES));

    MOCK_METHOD3(eglCreateSyncKHR, EGLSyncKHR(EGLDisplay, EGLenum, EGLint const*));
    MOCK_METHOD2(eglDestroySyncKHR, EGLBoolean(EGLDisplay, EGLSyncKHR));
    MOCK_METHOD4(eglClientWaitSyncKHR, EGLint(EGLDisplay, EGLSyncKHR, EGLint, EGLTimeKHR));

    MOCK_METHOD5(eglGetSyncValuesCHROMIUM, EGLBoolean(EGLDisplay, EGLSurface,
                                                      int64_t*, int64_t*,
                                                      int64_t*));

    MOCK_METHOD2(eglBindWaylandDisplayWL,
        EGLBoolean(EGLDisplay, struct wl_display*));
    MOCK_METHOD2(eglUnbindWaylandDisplayWL,
        EGLBoolean(EGLDisplay, struct wl_display*));
    MOCK_METHOD4(eglQueryWaylandBufferWL,
        EGLBoolean(EGLDisplay, struct wl_resource*, EGLint, EGLint*));

    EGLDisplay const fake_egl_display;
    EGLConfig const* const fake_configs;
    EGLint const fake_configs_num;
    EGLSurface const fake_egl_surface;
    EGLContext const fake_egl_context;
    EGLImageKHR const fake_egl_image;
    int const fake_visual_id;
    std::mutex mutable current_contexts_mutex;
    std::unordered_map<std::thread::id,EGLContext> current_contexts;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_EGL_H_ */
