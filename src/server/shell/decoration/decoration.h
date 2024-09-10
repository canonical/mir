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

#ifndef MIR_SHELL_DECORATION_DECORATION_H_
#define MIR_SHELL_DECORATION_DECORATION_H_

#include "mir_toolkit/event.h"
#include <string>

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

    // Called whenever the visible aspect of the window changes.
    // Examples: Scale, title, focus, visibility, size, etc...
    virtual void redraw() = 0;

    // Called whenever decorations receive input events
    // Mouse up, down, dragging, enter, leave...
    virtual void handle_input() = 0;

    virtual void request_toggle_maximize() = 0;
    virtual void request_close() = 0;
    virtual void request_minimize() = 0;
    virtual void request_move(MirInputEvent const*) = 0;
    virtual void request_resize(MirInputEvent const*, MirResizeEdge) = 0;

    virtual void set_cursor(std::string const& cursor_name) = 0;
    virtual void set_scale(float new_scale) = 0;

private:
    Decoration(Decoration const&) = delete;
    Decoration& operator=(Decoration const&) = delete;
};
}
}
}

#endif // MIR_SHELL_DECORATION_DECORATION_H_
