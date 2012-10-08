/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TOOLS_BUFFER_RENDER_TARGET_H_
#define MIR_TOOLS_BUFFER_RENDER_TARGET_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

namespace mir
{
namespace compositor
{
class Buffer;
}
namespace tools
{

class BufferRenderTarget
{
public:
    BufferRenderTarget(const mir::compositor::Buffer& buffer,
                       EGLClientBuffer client_buffer);

    void make_current();

private:
    class Resources
    {
    public:
        Resources()
            : egl_image{EGL_NO_IMAGE_KHR}, fbo{0},
              color_rbo{0}, depth_rbo{0}
        {
        }
        ~Resources();
        void setup(const mir::compositor::Buffer& buffer,
                   EGLClientBuffer client_buffer);

        EGLImageKHR egl_image;
        GLuint fbo;
        GLuint color_rbo;
        GLuint depth_rbo;
    };

    Resources resources;
    const mir::compositor::Buffer& buffer;
};

}
}

#endif /* MIR_TOOLS_BUFFER_RENDER_TARGET_H_ */
