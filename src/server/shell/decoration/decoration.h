/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_SHELL_DECORATION_DECORATION_H_
#define MIR_SHELL_DECORATION_DECORATION_H_

namespace mir
{
namespace shell
{
namespace decoration
{
/// Draws window decorations and provides basic move, resize, close, etc functionality
class Decoration
{
public:
    Decoration() = default;
    virtual ~Decoration() = default;

private:
    Decoration(Decoration const&) = delete;
    Decoration& operator=(Decoration const&) = delete;
};

typedef Decoration NullDecoration;
}
}
}

#endif // MIR_SHELL_DECORATION_DECORATION_H_
