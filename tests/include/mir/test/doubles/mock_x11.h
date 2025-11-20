/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_TEST_DOUBLES_MOCK_X11_H_
#define MIR_TEST_DOUBLES_MOCK_X11_H_

#include <gmock/gmock.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <xcb/xproto.h>

struct xcb_connection_t;

namespace mir
{
namespace test
{
namespace doubles
{

class FakeX11Resources
{
public:
    FakeX11Resources();
    ~FakeX11Resources() = default;

    Display *display;
    Window window;
    Screen screen;
    XVisualInfo visual_info;
    XEvent keypress_event_return = { 0 };
    XEvent key_release_event_return = { 0 };
    XEvent button_release_event_return = { 0 };
    XEvent expose_event_return = { 0 };
    XEvent focus_in_event_return = { 0 };
    XEvent focus_out_event_return = { 0 };
    XEvent vscroll_event_return = { 0 };
    XEvent motion_event_return = { 0 };
    XEvent enter_notify_event_return = { 0 };
    XEvent leave_notify_event_return = { 0 };
    int pending_events = 1;
};

class MockX11
{
public:
    MockX11();
    ~MockX11();

    MOCK_METHOD(Display*, XOpenDisplay, (const char*), ());
    MOCK_METHOD(int, XCloseDisplay, (Display*), ());
    MOCK_METHOD(int, XPending, (Display*), ());
    MOCK_METHOD(XVisualInfo*, XGetVisualInfo, (Display*, long, XVisualInfo*, int*), ());
    MOCK_METHOD(Colormap, XCreateColormap, (Display*, Window, Visual*, int), ());
    /* Too long to mock, use wrapper instead.
    MOCK_METHOD(Window, XCreateWindow, (Display*, Window, int, int, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual*, unsigned long, XSetWindowAttributes*), ());
    */
    MOCK_METHOD(Window, XCreateWindow_wrapper, (Display*, Window, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual*, unsigned long, XSetWindowAttributes*), ());
    MOCK_METHOD(int, XSetNormalHints, (Display*, Window, XSizeHints*), ());
    MOCK_METHOD(int, XSetStandardProperties, (Display*, Window, const char*, const char*, Pixmap, char **, int, XSizeHints*), ());
    MOCK_METHOD(int, XFree, (void*), ());
    MOCK_METHOD(int, XMapWindow, (Display*, Window), ());
    MOCK_METHOD(int, XDestroyWindow, (Display*, Window), ());
    MOCK_METHOD(int, XConnectionNumber, (Display*), ());
    MOCK_METHOD(int, XNextEvent, (Display*, XEvent*), ());
    MOCK_METHOD(int, XPeekEvent, (Display*, XEvent*), ());
    MOCK_METHOD(int, XEventsQueued, (Display*, int), ());
    MOCK_METHOD(int, XLookupString, (XKeyEvent*, char*, int, KeySym*, XComposeStatus*), ());
    MOCK_METHOD(int, XRefreshKeyboardMapping, (XMappingEvent*), ());
    MOCK_METHOD(Window, XDefaultRootWindow, (Display*), ());
    MOCK_METHOD(Screen*, XDefaultScreenOfDisplay, (Display*), ());
    MOCK_METHOD(int, XGrabKeyboard, (Display*, Window, Bool, int, int, Time), ());
    MOCK_METHOD(int, XUngrabKeyboard, (Display*, Time), ());
    MOCK_METHOD(int, XGetErrorText, (Display*, int, char*, int), ());
    MOCK_METHOD(XErrorHandler, XSetErrorHandler, (XErrorHandler), ());
    MOCK_METHOD(Status, XInitThreads, (), ());
    MOCK_METHOD(int, XSetWMHints, (Display*, Window, XWMHints*), ());
    MOCK_METHOD(int, XGrabPointer, (Display*, Window, Bool, unsigned int, int, int, Window, Cursor, Time), ());
    MOCK_METHOD(int, XUngrabPointer, (Display*, Time), ());
    MOCK_METHOD(Atom, XInternAtom, (Display*, const char*, Bool), ());
    MOCK_METHOD(Status, XSetWMProtocols, (Display*, Window, Atom*, int), ());
    MOCK_METHOD(Status, XGetGeometry, (Display*, Drawable, Window*, int*, int*, unsigned int*, unsigned int*, unsigned int*, unsigned int*), ());
    MOCK_METHOD(void, XFixesHideCursor, (Display *dpy, Window win), ());
    MOCK_METHOD(void, XFixesShowCursor, (Display *dpy, Window win), ());
    MOCK_METHOD(xcb_connection_t*, XGetXCBConnection, (Display *dpy), ());
    MOCK_METHOD(void, XSetEventQueueOwner, (Display*, int), ());

    MOCK_METHOD(int, xcb_connection_has_error, (xcb_connection_t*), ());
    MOCK_METHOD(xcb_screen_iterator_t, xcb_setup_roots_iterator, (xcb_setup_t const*), ());
    MOCK_METHOD(xcb_setup_t const*, xcb_get_setup, (xcb_connection_t*), ());
    MOCK_METHOD(xcb_intern_atom_cookie_t, xcb_intern_atom, (xcb_connection_t*, uint8_t, uint16_t, char const*), ());
    MOCK_METHOD(xcb_intern_atom_reply_t*, xcb_intern_atom_reply, (xcb_connection_t*, xcb_intern_atom_cookie_t, xcb_generic_error_t**), ());

    FakeX11Resources fake_x11;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_X11_H_ */
