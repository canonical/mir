/*
 * Copyright (C) 2021 Canonical Ltd.
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

#ifndef MIR_FRONTEND_XWAYLAND_CLIPBOARD_H_
#define MIR_FRONTEND_XWAYLAND_CLIPBOARD_H_

#include "xcb_connection.h"

namespace mir
{
namespace frontend
{
class XWaylandClipboard
{
public:
    XWaylandClipboard(std::shared_ptr<XCBConnection> const& connection);
    ~XWaylandClipboard();

private:
    XWaylandClipboard(XWaylandClipboard const&) = delete;
    XWaylandClipboard& operator=(XWaylandClipboard const&) = delete;

    std::shared_ptr<XCBConnection> const connection;
    xcb_window_t const selection_window;
};
}
}

#endif // MIR_FRONTEND_XWAYLAND_CLIPBOARD_H_
