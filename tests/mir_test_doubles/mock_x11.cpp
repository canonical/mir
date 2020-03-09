/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 * Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "mir/test/doubles/mock_x11.h"
#include <gtest/gtest.h>

#include <cstring>

namespace mtd=mir::test::doubles;

namespace
{
mtd::MockX11* global_mock = nullptr;
}

mtd::FakeX11Resources::FakeX11Resources()
    : display{reinterpret_cast<Display*>(0x12345678)},
      window{reinterpret_cast<Window>((long unsigned int)9876543210)}
{
    std::memset(&keypress_event_return, 0, sizeof(XEvent));
    std::memset(&button_release_event_return, 0, sizeof(XEvent));
    std::memset(&expose_event_return, 0, sizeof(XEvent));
    std::memset(&focus_in_event_return, 0, sizeof(XEvent));
    std::memset(&focus_out_event_return, 0, sizeof(XEvent));
    std::memset(&vscroll_event_return, 0, sizeof(XEvent));
    std::memset(&motion_event_return, 0, sizeof(XEvent));
    std::memset(&enter_notify_event_return, 0, sizeof(XEvent));
    std::memset(&leave_notify_event_return, 0, sizeof(XEvent));
    std::memset(&visual_info, 0, sizeof(XVisualInfo));
    std::memset(&screen, 0, sizeof screen);
    visual_info.red_mask = 0xFF0000;
    keypress_event_return.type = KeyPress;
    button_release_event_return.type = ButtonRelease;
    button_release_event_return.xbutton.button = 0;
    expose_event_return.type = Expose;
    focus_in_event_return.type = FocusIn;
    focus_out_event_return.type = FocusOut;
    vscroll_event_return.type = ButtonPress;
    XButtonEvent& xbev = (XButtonEvent&)vscroll_event_return;
    xbev.button = Button4;
    motion_event_return.type = MotionNotify;
    enter_notify_event_return.type = EnterNotify;
    leave_notify_event_return.type = LeaveNotify;
    screen.width = 2880;
    screen.height = 1800;
    screen.mwidth = 338;
    screen.mwidth = 270;
}

mtd::MockX11::MockX11()
{
    using namespace testing;
    assert(global_mock == nullptr && "Only one mock object per process is allowed");

    global_mock = this;

    ON_CALL(*this, XOpenDisplay(_))
    .WillByDefault(Return(fake_x11.display));

    ON_CALL(*this, XDefaultScreenOfDisplay(fake_x11.display))
    .WillByDefault(Return(&fake_x11.screen));

    ON_CALL(*this, XGetVisualInfo(fake_x11.display,_,_,_))
    .WillByDefault(DoAll(SetArgPointee<3>(1),
                         Return(&fake_x11.visual_info)));

    ON_CALL(*this, XCreateWindow_wrapper(fake_x11.display,_,_,_,_,_,_,_,_,_))
    .WillByDefault(Return(fake_x11.window));

    ON_CALL(*this, XInitThreads())
    .WillByDefault(Return(1));

    ON_CALL(*this, XPending(_))
    .WillByDefault(InvokeWithoutArgs([this]()
                                     {
                                         return fake_x11.pending_events;
                                     }));

    ON_CALL(*this, XGetGeometry(fake_x11.display,_,_,_,_,_,_,_,_))
    .WillByDefault(DoAll(SetArgPointee<5>(fake_x11.screen.width),
                         SetArgPointee<6>(fake_x11.screen.height),
                         Return(1)));
}

mtd::MockX11::~MockX11()
{
    global_mock = nullptr;
}

Display* XOpenDisplay(const char* display_name)
{
    return global_mock->XOpenDisplay(display_name);
}

int XCloseDisplay(Display* display)
{
    return global_mock->XCloseDisplay(display);
}

XVisualInfo* XGetVisualInfo(Display* display, long vinfo_mask, XVisualInfo* vinfo_template, int* nitems_return)
{
    return global_mock->XGetVisualInfo(display, vinfo_mask, vinfo_template, nitems_return);
}

Colormap XCreateColormap(Display* display, Window w, Visual* visual, int alloc)
{
    return global_mock->XCreateColormap(display, w, visual, alloc);
}

Window XCreateWindow(Display* display, Window parent, int /*x*/, int /*y*/, unsigned int width, unsigned int height, unsigned int border_width, int depth, unsigned int clss, Visual* visual, unsigned long valuemask, XSetWindowAttributes* attributes)
{
    return global_mock->XCreateWindow_wrapper(display, parent, width, height, border_width, depth, clss, visual, valuemask, attributes);
}

int XSetNormalHints(Display* display, Window w, XSizeHints* hints)
{
    return global_mock->XSetNormalHints(display, w, hints);
}

int XSetStandardProperties(Display* display, Window w, const char *window_name, const char *icon_name, Pixmap icon_pixmap, char** argv, int argc, XSizeHints* hints)
{
    return global_mock->XSetStandardProperties(display, w, window_name, icon_name, icon_pixmap, argv, argc, hints);
}

int XFree(void* data)
{
    return global_mock->XFree(data);
}

int XMapWindow(Display* display, Window w)
{
    return global_mock->XMapWindow(display, w);
}

int XDestroyWindow(Display* display, Window w)
{
    return global_mock->XDestroyWindow(display, w);
}

int XConnectionNumber(Display* display)
{
    return global_mock->XConnectionNumber(display);
}

int XNextEvent(Display* display, XEvent* event_return)
{
    auto const result = global_mock->XNextEvent(display, event_return);
    if (result) --global_mock->fake_x11.pending_events;
    return result;
}

int XLookupString(XKeyEvent* event_struct, char* buffer_return, int bytes_buffer, KeySym* keysym_return, XComposeStatus* status_in_out)
{
    return global_mock->XLookupString(event_struct, buffer_return, bytes_buffer, keysym_return, status_in_out);
}

int XRefreshKeyboardMapping(XMappingEvent* event_map)
{
    return global_mock->XRefreshKeyboardMapping(event_map);
}

Window XDefaultRootWindow(Display* display)
{
    return global_mock->XDefaultRootWindow(display);
}

int XGrabKeyboard(Display* display, Window grab_window, Bool owner_events, int pointer_mode, int keyboard_mode, Time time)
{
    return global_mock->XGrabKeyboard(display, grab_window, owner_events, pointer_mode, keyboard_mode, time);
}

int XUngrabKeyboard(Display* display, Time time)
{
    return global_mock->XUngrabKeyboard(display, time);
}

int XGetErrorText(Display* display, int code, char* buffer_return, int length)
{
    return global_mock->XGetErrorText(display, code, buffer_return, length);
}

XErrorHandler XSetErrorHandler(XErrorHandler handler)
{
    return global_mock->XSetErrorHandler(handler);
}

Status XInitThreads()
{
    return global_mock->XInitThreads();
}

int XSetWMHints(Display* display, Window window, XWMHints* wmhints)
{
    return global_mock->XSetWMHints(display, window, wmhints);
}

int XPending(Display* display)
{
    return global_mock->XPending(display);
}

int XGrabPointer(Display* display, Window grab_window, Bool owner_events,
                 unsigned int event_mask, int pointer_mode, int keyboard_mode,
                 Window confine_to, Cursor cursor, Time time)
{
    return global_mock->XGrabPointer(display, grab_window, owner_events, event_mask,
                                     pointer_mode, keyboard_mode, confine_to, cursor, time);
}

int XUngrabPointer(Display* display, Time time)
{
    return global_mock->XUngrabKeyboard(display, time);
}

Atom XInternAtom(Display* display, const char* atom_name, Bool only_if_exists)
{
    return global_mock->XInternAtom(display, atom_name, only_if_exists);
}

Status XSetWMProtocols(Display* display, Window w, Atom* protocols, int count)
{
    return global_mock->XSetWMProtocols(display, w, protocols, count);
}

Screen* XDefaultScreenOfDisplay(Display* display)
{
    return global_mock->XDefaultScreenOfDisplay(display);
}

Status XGetGeometry(Display* display, Drawable d,
                    Window* root_return,
                    int* x_return, int* y_return,
                    unsigned int* width_return, unsigned int* height_return,
                    unsigned int* border_width_return, unsigned int* depth_return)
{
    return global_mock->XGetGeometry(display, d, root_return,
                                     x_return, y_return,
                                     width_return, height_return,
                                     border_width_return, depth_return);
}

extern "C" void XFixesHideCursor(Display *dpy, Window win)
{
    global_mock->XFixesHideCursor(dpy, win);
}

extern "C" void XFixesShowCursor(Display *dpy, Window win)
{
    global_mock->XFixesShowCursor(dpy, win);
}
