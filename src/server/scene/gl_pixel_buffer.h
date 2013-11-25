/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored By: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_SCENE_GL_PIXEL_BUFFER_H_
#define MIR_SCENE_GL_PIXEL_BUFFER_H_

#include "pixel_buffer.h"

#include <memory>
#include <vector>

#include <GLES2/gl2.h>

namespace mir
{
namespace graphics
{
class Buffer;
class GLContext;
}

namespace scene
{
/** Extracts the pixels from a graphics::Buffer using GL facilities. */
class GLPixelBuffer : public PixelBuffer
{
public:
    GLPixelBuffer(std::unique_ptr<graphics::GLContext> gl_context);
    ~GLPixelBuffer() noexcept;

    void fill_from(graphics::Buffer& buffer);
    void const* as_argb_8888();
    geometry::Size size() const;
    geometry::Stride stride() const;

private:
    void prepare();
    void copy_and_convert_pixel_line(char* src, char* dst);

    std::unique_ptr<graphics::GLContext> const gl_context;
    GLuint tex;
    GLuint fbo;
    std::vector<char> pixels;
    GLuint gl_pixel_format;
    bool pixels_need_y_flip;
    geometry::Size size_;
    geometry::Stride stride_;
};

}
}

#endif /* MIR_SCENE_GL_PIXEL_BUFFER_H_ */
