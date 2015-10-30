/*
 * EGL without any linkage requirements!
 * ~~~
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_CLIENT_WEAK_EGL_H_
#define MIR_CLIENT_WEAK_EGL_H_

#include <EGL/egl.h>
#include <dlfcn.h>

namespace mir { namespace client {

class WeakEGL
{
public:
    WeakEGL();
    ~WeakEGL();
    EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
                                  EGLint attribute, EGLint* value);

private:
    bool find(char const* name, void** func);

    void* egl1;
    EGLBoolean (*pGetConfigAttrib)(EGLDisplay dpy, EGLConfig config,
                                   EGLint attribute, EGLint* value);
};

}} // namespace mir::client

#endif // MIR_CLIENT_WEAK_EGL_H_
