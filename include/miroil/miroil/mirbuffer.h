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
}

namespace miroil
{
class GLBuffer
{
public:
    ~GLBuffer();
    explicit GLBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer);

    bool has_alpha_channel() const;
    mir::geometry::Size size() const;
    void bind();

    void reset(std::shared_ptr<mir::graphics::Buffer> const& buffer);

    void reset();
    bool empty();

    static std::shared_ptr<GLBuffer> from_mir_buffer(std::shared_ptr<mir::graphics::Buffer> const& buffer);

private:
    std::shared_ptr<mir::graphics::Buffer> wrapped;
};
}

#endif //MIROIL_GLBUFFER_H
