/*
 * Copyright © 2016 Canonical Ltd.
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
 * Author: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_PLAYGROUND_MIR_EGL_PLATFORM_SHIM_H_
#define MIR_PLAYGROUND_MIR_EGL_PLATFORM_SHIM_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "mir_toolkit/rs/mir_render_surface.h"

//Note that these have the same signatures as the proper EGL functions,
//and use our intended EGLNativeDisplayType and EGLNativeWindowType.

EGLDisplay future_driver_eglGetDisplay(MirConnection*);

EGLBoolean future_driver_eglTerminate(EGLDisplay);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
EGLSurface future_driver_eglCreateWindowSurface(
    EGLDisplay display, EGLConfig config, MirRenderSurface* surface, const EGLint *);
#pragma GCC diagnostic pop

EGLBoolean future_driver_eglSwapBuffers(EGLDisplay display, EGLSurface surface);

EGLImageKHR future_driver_eglCreateImageKHR(
    EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);

EGLBoolean future_driver_eglDestroyImageKHR (EGLDisplay dpy, EGLImageKHR image);

void future_driver_glEGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image);

#endif /* MIR_PLAYGROUND_MIR_EGL_PLATFORM_SHIM_H_*/
