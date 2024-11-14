/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_SHELL_DECORATION_H_
#define MIR_SHELL_DECORATION_H_

#include "mir/geometry/forward.h"
#include "mir_toolkit/common.h"

namespace mir::shell::decoration
{
/// Draws window decorations and provides basic move, resize, close, etc functionality
class Decoration
{
public:
    Decoration() = default;
    virtual ~Decoration() = default;

    virtual void set_scale(float new_scale) = 0;

private:
    Decoration(Decoration const&) = delete;
    Decoration& operator=(Decoration const&) = delete;
};

auto compute_size_with_decorations(geometry::Size content_size, MirWindowType type, MirWindowState state)
    -> geometry::Size;
}

#endif
