/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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

namespace mtd=mir::test::doubles;

namespace
{
mtd::MockX11* global_mock = nullptr;
}

mtd::FakeX11Resources::FakeX11Resources()
    : display{reinterpret_cast<Display*>(0x12345678)},
      window{reinterpret_cast<Window>((long unsigned int)9876543210)}
{
    visual_info.depth=24;
    keypress_event_return.type = KeyPress;
    expose_event_return.type = Expose;
    focus_in_event_return.type = FocusIn;
    focus_out_event_return.type = FocusOut;
}

mtd::MockX11::MockX11()
{
    using namespace testing;
    assert(global_mock == nullptr && "Only one mock object per process is allowed");

    global_mock = this;

    ON_CALL(*this, XOpenDisplay(_))
    .WillByDefault(Return(fake_x11.display));

    ON_CALL(*this, XGetVisualInfo(fake_x11.display,_,_,_))
    .WillByDefault(Return(&fake_x11.visual_info));

    ON_CALL(*this, XCreateWindow_wrapper(fake_x11.display,_,_,_,_,_,_,_,_,_))
    .WillByDefault(Return(fake_x11.window));

    ON_CALL(*this, XInitThreads())
    .WillByDefault(Return(1));
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
    return global_mock->XNextEvent(display, event_return);
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
