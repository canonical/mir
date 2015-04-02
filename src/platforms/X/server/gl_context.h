/*
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *
 */

#ifndef MIR_GRAPHICS_X_GL_CONTEXT_H_
#define MIR_GRAPHICS_X_GL_CONTEXT_H_

#include<X11/X.h>
#include<X11/Xlib.h>
#include<GL/gl.h>
#include<GL/glx.h>
#include<GL/glu.h>

#include "mir/graphics/gl_context.h"

namespace mir
{
namespace graphics
{
namespace X
{

class XGLContext : public graphics::GLContext
{
public:
	XGLContext(::Display* const display, Window const win, GLXContext const glc);
	~XGLContext();
    void make_current() const override;
    void release_current() const override;

private:
    ::Display *dpy;
    Window     win;
    GLXContext glc;
};

}
}
}

#endif /* MIR_GRAPHICS_X_GL_CONTEXT_H_ */
