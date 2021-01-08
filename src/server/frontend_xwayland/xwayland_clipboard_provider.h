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

#ifndef MIR_FRONTEND_XWAYLAND_CLIPBOARD_PROVIDER_H_
#define MIR_FRONTEND_XWAYLAND_CLIPBOARD_PROVIDER_H_

#include "xcb_connection.h"

#include <mutex>

namespace mir
{
namespace scene
{
class Clipboard;
class ClipboardSource;
}
namespace frontend
{
/// Exposes non-X11 selections to X11 clients
class XWaylandClipboardProvider
{
public:
    XWaylandClipboardProvider(
        XCBConnection& connection,
        std::shared_ptr<scene::Clipboard> const& clipboard);
    ~XWaylandClipboardProvider();

    /// Called by the observer, indicates the paste source has been set by someone (could be us or Wayland)
    void paste_source_set(std::shared_ptr<scene::ClipboardSource> const& source);

private:
    class ClipboardObserver;

    XWaylandClipboardProvider(XWaylandClipboardProvider const&) = delete;
    XWaylandClipboardProvider& operator=(XWaylandClipboardProvider const&) = delete;

    XCBConnection& connection;
    std::shared_ptr<scene::Clipboard> const clipboard;
    std::shared_ptr<ClipboardObserver> const clipboard_observer;
    xcb_window_t const selection_window;

    std::mutex mutex;
    bool owns_x11_clipboard{false};
};
}
}

#endif // MIR_FRONTEND_XWAYLAND_CLIPBOARD_PROVIDER_H_
