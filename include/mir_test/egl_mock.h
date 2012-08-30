#ifndef EGL_MOCK_H_
#define EGL_MOCK_H_

#include <gmock/gmock.h>

#include <EGL/egl.h>

namespace mir
{
class EglMock
{
public:
    EglMock();
    ~EglMock();
    void silence_uninteresting();

    MOCK_METHOD0(eglGetError, EGLint (void));

    MOCK_METHOD1(eglGetDisplay, EGLDisplay(NativeDisplayType));
    MOCK_METHOD3(eglInitialize, EGLBoolean(EGLDisplay,EGLint*,EGLint*));
    MOCK_METHOD1(eglTerminate, EGLBoolean(EGLDisplay));
    MOCK_METHOD2(eglQueryString,const char*(EGLDisplay, EGLint));

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

    EGLDisplay fake_egl_display;
    EGLConfig* fake_configs;
    EGLint fake_configs_num;
    EGLSurface fake_egl_surface;
    EGLContext fake_egl_context;
    int fake_visual_id;
};
}

#endif // EGL_MOCK_H_
