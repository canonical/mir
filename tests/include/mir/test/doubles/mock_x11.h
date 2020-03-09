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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_X11_H_
#define MIR_TEST_DOUBLES_MOCK_X11_H_

#include <gmock/gmock.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

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

    MOCK_METHOD1(XOpenDisplay, Display*(const char*));
    MOCK_METHOD1(XCloseDisplay, int(Display*));
    MOCK_METHOD1(XPending, int(Display*));
    MOCK_METHOD4(XGetVisualInfo, XVisualInfo*(Display*, long, XVisualInfo*, int*));
    MOCK_METHOD4(XCreateColormap, Colormap(Display*, Window, Visual*, int));
    /* Too long to mock, use wrapper instead.
    MOCK_METHOD12(XCreateWindow, Window(Display*, Window, int, int, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual*, unsigned long, XSetWindowAttributes*));
    */
    MOCK_METHOD10(XCreateWindow_wrapper, Window(Display*, Window, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual*, unsigned long, XSetWindowAttributes*));
    MOCK_METHOD3(XSetNormalHints, int(Display*, Window, XSizeHints*));
    MOCK_METHOD8(XSetStandardProperties, int(Display*, Window, const char*, const char*, Pixmap, char **, int, XSizeHints*));
    MOCK_METHOD1(XFree, int(void*));
    MOCK_METHOD2(XMapWindow, int(Display*, Window));
    MOCK_METHOD2(XDestroyWindow, int(Display*, Window));
    MOCK_METHOD1(XConnectionNumber, int(Display*));
    MOCK_METHOD2(XNextEvent, int(Display*, XEvent*));
    MOCK_METHOD5(XLookupString, int(XKeyEvent*, char*, int, KeySym*, XComposeStatus*));
    MOCK_METHOD1(XRefreshKeyboardMapping, int(XMappingEvent*));
    MOCK_METHOD1(XDefaultRootWindow, Window(Display*));
    MOCK_METHOD1(XDefaultScreenOfDisplay, Screen*(Display*));
    MOCK_METHOD6(XGrabKeyboard, int(Display*, Window, Bool, int, int, Time));
    MOCK_METHOD2(XUngrabKeyboard, int(Display*, Time));
    MOCK_METHOD4(XGetErrorText, int(Display*, int, char*, int ));
    MOCK_METHOD1(XSetErrorHandler, XErrorHandler(XErrorHandler));
    MOCK_METHOD0(XInitThreads, Status());
    MOCK_METHOD3(XSetWMHints, int(Display*, Window, XWMHints*));
    MOCK_METHOD9(XGrabPointer, int(Display*, Window, Bool, unsigned int, int, int, Window, Cursor, Time));
    MOCK_METHOD2(XUngrabPointer, int(Display*, Time));
    MOCK_METHOD3(XInternAtom, Atom(Display*, const char*, Bool));
    MOCK_METHOD4(XSetWMProtocols, Status(Display*, Window, Atom*, int));
    MOCK_METHOD9(XGetGeometry, Status(Display*, Drawable, Window*, int*, int*, unsigned int*, unsigned int*, unsigned int*, unsigned int*));
    MOCK_METHOD2(XFixesHideCursor, void(Display *dpy, Window win));
    MOCK_METHOD2(XFixesShowCursor, void(Display *dpy, Window win));

    FakeX11Resources fake_x11;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_X11_H_ */
