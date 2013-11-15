/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_GL_CONTEXT_H_
#define MIR_GRAPHICS_GL_CONTEXT_H_

namespace mir
{
namespace graphics
{

class GLContext
{
public:
    virtual ~GLContext() = default;

    virtual void make_current() = 0;
    virtual void release_current() = 0;

protected:
    GLContext() = default;
    GLContext(GLContext const&) = delete;
    GLContext& operator=(GLContext const&) = delete;
};

}
}

#endif /* MIR_GRAPHICS_GL_CONTEXT_H_ */
