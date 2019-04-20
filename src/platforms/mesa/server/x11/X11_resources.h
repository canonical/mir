/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_X11_RESOURCES_H_
#define MIR_X11_RESOURCES_H_

#include <X11/Xlib.h>

namespace mir
{
namespace X
{

int mir_x11_error_handler(Display* dpy, XErrorEvent* eev);

class X11Resources
{
public:
    std::shared_ptr<::Display> get_conn();

    static X11Resources instance;

private:
    std::weak_ptr<::Display> connection;
};

}
}
#endif /* MIR_X11_RESOURCES_H_ */
