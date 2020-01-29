/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_COMPOSITOR_DISPLAY_LISTENER_H_
#define MIR_COMPOSITOR_DISPLAY_LISTENER_H_

namespace mir
{
namespace geometry { struct Rectangle; }
namespace compositor
{
class DisplayListener
{
public:
    virtual void add_display(geometry::Rectangle const& area) = 0;

    virtual void remove_display(geometry::Rectangle const& area) = 0;

protected:
    DisplayListener() = default;
    virtual ~DisplayListener() = default;
    DisplayListener(DisplayListener const&) = delete;
    DisplayListener& operator=(DisplayListener const&) = delete;
};
}
}

#endif /* MIR_COMPOSITOR_DISPLAY_LISTENER_H_ */
