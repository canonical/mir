/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_EXAMPLES_EGL_HELPER_H_
#define MIR_EXAMPLES_EGL_HELPER_H_

#include <EGL/egl.h>

namespace mir
{
namespace examples
{

/// Simple helper class for managing an EGL rendering with a sensible default configuration.
class EGLHelper
{
public:
    EGLHelper(EGLNativeDisplayType native_display, EGLNativeWindowType native_surface);
    virtual ~EGLHelper();
    
    EGLDisplay the_display() const;
    EGLContext the_context() const;
    EGLSurface the_surface() const;

protected:
    EGLHelper(EGLHelper const&) = delete;
    EGLHelper& operator=(EGLHelper const&) = delete;

private:
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
};

}
} // namespace mir

#endif // MIR_EXAMPLES_EGL_HELPER_H_
