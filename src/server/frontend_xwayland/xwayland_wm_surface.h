/*
 * Copyright (C) 2018 Marius Gripsgard <marius@ubports.com>
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
 */

#ifndef MIR_FRONTEND_XWAYLAND_WM_SHELLSURFACE_H
#define MIR_FRONTEND_XWAYLAND_WM_SHELLSURFACE_H

#include "wl_surface.h"
#include "xwayland_wm.h"

#include <mutex>

extern "C" {
#include <xcb/xcb.h>
}

struct wm_size_hints
{
    uint32_t flags;
    int32_t x, y;
    int32_t width, height; /* should set so old wm's don't mess up */
    int32_t min_width, min_height;
    int32_t max_width, max_height;
    int32_t width_inc, height_inc;
    struct
    {
        int32_t x;
        int32_t y;
    } min_aspect, max_aspect;
    int32_t base_width, base_height;
    int32_t win_gravity;
};

#define USPosition (1L << 0)
#define USSize (1L << 1)
#define PPosition (1L << 2)
#define PSize (1L << 3)
#define PMinSize (1L << 4)
#define PMaxSize (1L << 5)
#define PResizeInc (1L << 6)
#define PAspect (1L << 7)
#define PBaseSize (1L << 8)
#define PWinGravity (1L << 9)

struct motif_wm_hints
{
    uint32_t flags;
    uint32_t functions;
    uint32_t decorations;
    int32_t input_mode;
    uint32_t status;
};

#define MWM_HINTS_FUNCTIONS (1L << 0)
#define MWM_HINTS_DECORATIONS (1L << 1)
#define MWM_HINTS_INPUT_MODE (1L << 2)
#define MWM_HINTS_STATUS (1L << 3)

#define MWM_FUNC_ALL (1L << 0)
#define MWM_FUNC_RESIZE (1L << 1)
#define MWM_FUNC_MOVE (1L << 2)
#define MWM_FUNC_MINIMIZE (1L << 3)
#define MWM_FUNC_MAXIMIZE (1L << 4)
#define MWM_FUNC_CLOSE (1L << 5)

#define MWM_DECOR_ALL (1L << 0)
#define MWM_DECOR_BORDER (1L << 1)
#define MWM_DECOR_RESIZEH (1L << 2)
#define MWM_DECOR_TITLE (1L << 3)
#define MWM_DECOR_MENU (1L << 4)
#define MWM_DECOR_MINIMIZE (1L << 5)
#define MWM_DECOR_MAXIMIZE (1L << 6)

#define MWM_DECOR_EVERYTHING \
    (MWM_DECOR_BORDER | MWM_DECOR_RESIZEH | MWM_DECOR_TITLE | MWM_DECOR_MENU | MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE)

#define MWM_INPUT_MODELESS 0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL 2
#define MWM_INPUT_FULL_APPLICATION_MODAL 3
#define MWM_INPUT_APPLICATION_MODAL MWM_INPUT_PRIMARY_APPLICATION_MODAL

#define MWM_TEAROFF_WINDOW (1L << 0)

#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT 0
#define _NET_WM_MOVERESIZE_SIZE_TOP 1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT 2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT 3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT 4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM 5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT 6
#define _NET_WM_MOVERESIZE_SIZE_LEFT 7
#define _NET_WM_MOVERESIZE_MOVE 8           /* movement only */
#define _NET_WM_MOVERESIZE_SIZE_KEYBOARD 9  /* size via keyboard */
#define _NET_WM_MOVERESIZE_MOVE_KEYBOARD 10 /* move via keyboard */
#define _NET_WM_MOVERESIZE_CANCEL 11        /* cancel operation */

#define TYPE_WM_PROTOCOLS XCB_ATOM_CUT_BUFFER0
#define TYPE_MOTIF_WM_HINTS XCB_ATOM_CUT_BUFFER1
#define TYPE_NET_WM_STATE XCB_ATOM_CUT_BUFFER2
#define TYPE_WM_NORMAL_HINTS XCB_ATOM_CUT_BUFFER3

namespace mir
{
namespace frontend
{
class XWaylandWM;
class XWaylandWMShellSurface;
class XWaylandWMSurface
{
public:
    enum WmState
    {
        WithdrawnState = 0,
        NormalState = 1,
        IconicState = 3
    };

    XWaylandWMSurface(XWaylandWM *wm, xcb_create_notify_event_t *event);
    ~XWaylandWMSurface();
    void dirty_properties();
    void read_properties();
    void set_surface(WlSurface* wayland_surface); ///< Should only be called on the Wayland thread
    void set_workspace(int workspace);
    void set_wm_state(WmState state);
    void set_net_wm_state();
    void move_resize(uint32_t detail);
    void send_resize(const geometry::Size& new_size);
    void send_close_request();

private:
    /// Runs work on the Wayland thread if the shell surface hasn't been destroyed
    void aquire_shell_surface(std::function<void(XWaylandWMShellSurface* shell_surface)>&& work);

    XWaylandWM* const xwm;
    xcb_window_t const window;

    std::mutex mutex;

    bool props_dirty;
    bool maximized;
    bool fullscreen;
    struct
    {
        std::string title;
        std::string appId;
        int deleteWindow;
    } properties;

    struct
    {
        xcb_window_t parent;
        geometry::Point position;
        geometry::Size size;
        bool override_redirect;
    } const init;

    /// shell_surface should not be accessed unless this is false
    /// Should only be accessed on the Wayland thread
    std::shared_ptr<bool> shell_surface_destroyed;

    /// Only safe to access when shell_surface_destroyed is false and on the Wayland thread (use aquire_shell_surface())
    /// When the associated wl_surface is destroyed:
    /// - The WlSurface will call destory() on its role (this shell surface)
    /// - shell_surface_destroyed will be set to true
    /// - The shell surface will delete itself
    /// Can be deleted on the Wayland thread at any time (which will result in shell_surface_destroyed being set to true)
    XWaylandWMShellSurface* shell_surface_unsafe;
};
} /* frontend */
} /* mir */

#endif /* end of include guard: MIR_FRONTEND_XWAYLAND_WM_SHELLSURFACE_H */
