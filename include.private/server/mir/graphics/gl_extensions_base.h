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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_GL_EXTENSIONS_BASE_H_
#define MIR_GRAPHICS_GL_EXTENSIONS_BASE_H_

namespace mir
{
namespace graphics
{

class GLExtensionsBase
{
public:
    GLExtensionsBase(char const* extensions);

    bool support(char const* ext) const;

private:
    GLExtensionsBase(GLExtensionsBase const&) = delete;
    GLExtensionsBase& operator=(GLExtensionsBase const&) = delete;

    char const* const extensions;
};

}
}

#endif /* MIR_GRAPHICS_GL_EXTENSIONS_BASE_H_ */
