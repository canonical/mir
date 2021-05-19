/*
 * Copyright © 2017 Canonical Ltd.
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
 */

#ifndef MIROIL_GLBUFFER_H
#define MIROIL_GLBUFFER_H

#include <mir/geometry/size.h>

#include <memory>

namespace mir { namespace graphics { class Buffer; }}

namespace miroil
{
class GLBuffer
{
public:
    enum Type {
        GLTexture = 0,
        GLTextureSource = 1,
    };

    virtual ~GLBuffer();
    explicit GLBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer);

    bool has_alpha_channel() const;
    mir::geometry::Size size() const;

    virtual void bind() = 0;
    virtual Type type() = 0;

    void reset();
    void reset(std::shared_ptr<mir::graphics::Buffer> const& buffer);
    bool empty();

    static std::shared_ptr<GLBuffer> from_mir_buffer(std::shared_ptr<mir::graphics::Buffer> const& buffer);

protected:
    std::shared_ptr<mir::graphics::Buffer> wrapped;
};

class GLTextureSourceBuffer : public GLBuffer
{
public:
    GLTextureSourceBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer);
    void bind() override;
    Type type() override { return Type::GLTextureSource; };
};

class GLTextureBuffer : public GLBuffer
{
public:
    GLTextureBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer);
    void bind() override;
    Type type() override { return Type::GLTexture; };
};

}

#endif //MIROIL_GLBUFFER_H
