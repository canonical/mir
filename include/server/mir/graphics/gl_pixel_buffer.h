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

#ifndef MIR_GRAPHICS_GL_PIXEL_BUFFER_H_
#define MIR_GRAPHICS_GL_PIXEL_BUFFER_H_

#include "mir/shell/pixel_buffer.h"

#include <memory>
#include <vector>

#include <GLES2/gl2.h>

namespace mir
{
namespace graphics
{
class Buffer;
class GLContext;

/** Extracts the pixels from a compositor::Buffer using GL facilities. */
class GLPixelBuffer : public shell::PixelBuffer
{
public:
    GLPixelBuffer(std::unique_ptr<GLContext> gl_context);
    ~GLPixelBuffer() noexcept;

    void fill_from(Buffer& buffer);
    void const* as_argb_8888();
    geometry::Size size() const;
    geometry::Stride stride() const;

private:
    void prepare();

    std::unique_ptr<GLContext> const gl_context;
    GLuint tex;
    GLuint fbo;
    std::vector<char> pixels;
    bool pixels_need_y_flip;
    geometry::Size size_;
    geometry::Stride stride_;
};

}
}

#endif /* MIR_GRAPHICS_GL_PIXEL_BUFFER_H_ */
