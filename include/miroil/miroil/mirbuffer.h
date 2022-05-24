/*
 * Copyright © 2017-2020 Canonical Ltd.
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
#include <mir/version.h>
#include <memory>

namespace mir {
    namespace graphics {
        class Buffer;

        namespace gl {
            class Texture;
        }
    }

#if MIR_SERVER_VERSION < MIR_VERSION_NUMBER(2, 3, 0)
    namespace renderer {
        namespace gl {
            class TextureSource;
        }
    }
#endif
}

namespace miroil
{
class GLBuffer
{
public:
    enum Type {
        GLTexture = 0,
#if MIR_SERVER_VERSION < MIR_VERSION_NUMBER(2, 3, 0)
        GLTextureSource = 1,
#endif
    };

    virtual ~GLBuffer();
    explicit GLBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer);

    bool has_alpha_channel() const;
    mir::geometry::Size size() const;

    virtual void setWrapped(std::shared_ptr<mir::graphics::Buffer> const& buffer);

    virtual Type type() = 0;

    void reset();
    bool empty();

    static std::shared_ptr<GLBuffer> from_mir_buffer(std::shared_ptr<mir::graphics::Buffer> const& buffer);

protected:
    std::shared_ptr<mir::graphics::Buffer> wrapped;
};

#if MIR_SERVER_VERSION < MIR_VERSION_NUMBER(2, 3, 0)
class GLTextureSourceBuffer : public GLBuffer
{
public:
    GLTextureSourceBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer);

    void setWrapped(std::shared_ptr<mir::graphics::Buffer> const& buffer) override;

    void upload_to_texture();

    Type type() override { return Type::GLTextureSource; };

private:
    mir::renderer::gl::TextureSource* m_texSource;
};
#endif

class GLTextureBuffer : public GLBuffer
{
public:
    GLTextureBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer);

    void setWrapped(std::shared_ptr<mir::graphics::Buffer> const& buffer) override;

    void tex_bind();

    Type type() override { return Type::GLTexture; };

private:
    mir::graphics::gl::Texture *m_mirTex;
};

}

#endif //MIRAL_GLBUFFER_H
