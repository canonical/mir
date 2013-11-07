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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_EGL_H_
#define MIR_TEST_DOUBLES_MOCK_EGL_H_

#include <gmock/gmock.h>

#define GL_GLEXT_PROTOTYPES
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

class MockEGL
{
public:
    MockEGL();
    ~MockEGL();
    void silence_uninteresting();

    typedef void (*generic_function_pointer_t)(void);

    MOCK_METHOD1(eglGetDisplay, EGLDisplay(NativeDisplayType));
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
    MOCK_METHOD4(eglCreateWindowSurface, EGLSurface(EGLDisplay,EGLConfig,NativeWindowType,const EGLint*));
    MOCK_METHOD4(eglCreatePixmapSurface, EGLSurface(EGLDisplay,EGLConfig,NativePixmapType,const EGLint*));
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
    MOCK_METHOD3(eglCopyBuffers, EGLBoolean(EGLDisplay,EGLSurface,NativePixmapType));

    MOCK_METHOD0(eglGetError, EGLint (void));

    MOCK_METHOD5(eglCreateImageKHR, EGLImageKHR(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLint*));
    MOCK_METHOD2(eglDestroyImageKHR,EGLBoolean(EGLDisplay, EGLImageKHR));
    MOCK_METHOD2(glEGLImageTargetTexture2DOES, void(GLenum, GLeglImageOES));

    EGLDisplay fake_egl_display;
    EGLConfig* fake_configs;
    EGLint fake_configs_num;
    EGLSurface fake_egl_surface;
    EGLContext fake_egl_context;
    EGLImageKHR fake_egl_image;
    int fake_visual_id;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_EGL_H_ */
