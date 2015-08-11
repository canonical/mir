/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_X_LAZY_CONNECTION_H_
#define MIR_X_LAZY_CONNECTION_H_

#include <X11/Xlib.h>

namespace mir
{
namespace X
{

class LazyConnection
{
public:
    std::shared_ptr<::Display> get()
    {
        if (auto conn = connection.lock())
            return conn;

        std::shared_ptr<::Display> new_conn{
            XOpenDisplay(nullptr),
            [](::Display* display) { XCloseDisplay(display); }};

        connection = new_conn;
        return new_conn;
    }

private:
    std::weak_ptr<::Display> connection;
};

}
}
#endif /* MIR_X_LAZY_CONNECTION_H_ */
