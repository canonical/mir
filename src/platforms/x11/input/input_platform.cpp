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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "input_platform.h"
#include "input_device.h"

#include "mir/graphics/display_configuration.h"
#include "mir/input/input_device_registry.h"
#include "mir/input/input_device_info.h"
#include "mir/dispatch/readable_fd.h"
#include "../X11_resources.h"

#define MIR_LOG_COMPONENT "x11-input"
#include "mir/log.h"

#include <X11/extensions/Xfixes.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <linux/input.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>

#include <chrono>
// Uncomment for verbose output with log_info.
//#define MIR_ON_X11_INPUT_VERBOSE

// Due to a bug in Unity when keyboard is grabbed,
// client cannot be resized. This helps in debugging.
#define GRAB_KBD

namespace mi = mir::input;
namespace geom = mir::geometry;
namespace md = mir::dispatch;
namespace mix = mi::X;
namespace mx = mir::X;

namespace
{
geom::Point get_pos_on_output(Window x11_window, int x, int y)
{
    auto output = mx::X11Resources::instance.get_output_config_for_win(x11_window);
    if (output)
    {
        return output.value()->top_left + geom::Displacement{x, y};
    }
    else
    {
        mir::log_warning("X11 window %d does not map to any known output, not applying input transformation", x11_window);
        return geom::Point{x, y};
    }
}
}

mix::XInputPlatform::XInputPlatform(std::shared_ptr<mi::InputDeviceRegistry> const& input_device_registry,
                                    std::shared_ptr<::Display> const& conn) :
    x11_connection{conn},
    xcon_dispatchable(std::make_shared<md::ReadableFd>(
                                mir::Fd{mir::IntOwnedFd{XConnectionNumber(conn.get())}}, [this]()
                                {
                                    process_input_event();
                                })),
    registry(input_device_registry),
    core_keyboard(std::make_shared<mix::XInputDevice>(
            mi::InputDeviceInfo{"x11-keyboard-device", "x11-key-dev-1", mi::DeviceCapability::keyboard})),
    core_pointer(std::make_shared<mix::XInputDevice>(
            mi::InputDeviceInfo{"x11-mouse-device", "x11-mouse-dev-1", mi::DeviceCapability::pointer})),
    kbd_grabbed{false},
    ptr_grabbed{false}
{
}

void mix::XInputPlatform::start()
{
    registry->add_device(core_keyboard);
    registry->add_device(core_pointer);
}

std::shared_ptr<md::Dispatchable> mix::XInputPlatform::dispatchable()
{
    return xcon_dispatchable;
}

void mix::XInputPlatform::stop()
{
    registry->remove_device(core_keyboard);
    registry->remove_device(core_pointer);
}

void mix::XInputPlatform::pause_for_config()
{
}

void mix::XInputPlatform::continue_after_config()
{
}

void mix::XInputPlatform::process_input_event()
{
    while(XPending(x11_connection.get()))
    {
        // This code is based on :
        // https://tronche.com/gui/x/xlib/events/keyboard-pointer/keyboard-pointer.html
        XEvent xev;

        XNextEvent(x11_connection.get(), &xev);

        if (core_keyboard->started() && core_pointer->started())
        {
            switch (xev.type)
            {
#ifdef GRAB_KBD
            case FocusIn:
                {
                    auto const& xfiev = xev.xfocus;
                    if (!kbd_grabbed && (xfiev.mode == NotifyNormal || xfiev.mode == NotifyWhileGrabbed))
                    {
                        XGrabKeyboard(xfiev.display, xfiev.window, True, GrabModeAsync, GrabModeAsync, CurrentTime);
                        kbd_grabbed = true;
                    }
                    break;
                }

            case FocusOut:
                {
                    auto const& xfoev = xev.xfocus;
                    if (kbd_grabbed && (xfoev.mode == NotifyNormal || xfoev.mode == NotifyWhileGrabbed))
                    {
                        XUngrabKeyboard(xfoev.display, CurrentTime);
                        kbd_grabbed = false;
                    }
                    break;
                }
#endif
            case EnterNotify:
                {
                    if (!ptr_grabbed && kbd_grabbed)
                    {
                        auto const& xenev = xev.xcrossing;
                        XGrabPointer(xenev.display, xenev.window, True, 0, GrabModeAsync,
                                     GrabModeAsync, None, None, CurrentTime);
                        XFixesHideCursor(xenev.display, xenev.window);
                        ptr_grabbed = true;
                    }
                    break;
                }

            case LeaveNotify:
                {
                    if (ptr_grabbed)
                    {
                        auto const& xlnev = xev.xcrossing;
                        XUngrabPointer(xlnev.display, CurrentTime);
                        XFixesShowCursor(xlnev.display, xlnev.window);
                        ptr_grabbed = false;
                    }
                    break;
                }

            case KeyPress:
            case KeyRelease:
                {
                    auto & xkev = xev.xkey;
                    static const int STRMAX = 32;
                    char str[STRMAX];
                    KeySym keysym;

#ifdef MIR_ON_X11_INPUT_VERBOSE
                    mir::log_info("X11 key event :"
                                  " type=%s, serial=%u, send_event=%d, display=%p, window=%p,"
                                  " root=%p, subwindow=%p, time=%d, x=%d, y=%d, x_root=%d,"
                                  " y_root=%d, state=0x%0X, keycode=%d, same_screen=%d",
                                  xkev.type == KeyPress ? "down" : "up", xkev.serial,
                                  xkev.send_event, xkev.display, xkev.window, xkev.root,
                                  xkev.subwindow, xkev.time, xkev.x, xkev.y, xkev.x_root,
                                  xkev.y_root, xkev.state, xkev.keycode, xkev.same_screen);
                    auto count =
#endif
                        XLookupString(&xkev, str, STRMAX, &keysym, NULL);

                    auto const event_time =
                        std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::milliseconds{xkev.time});

#ifdef MIR_ON_X11_INPUT_VERBOSE
                    for (int i=0; i<count; i++)
                        mir::log_info("buffer[%d]='%c'", i, str[i]);
                    mir::log_info("Mir key event : "
                                  "key_code=%d, scan_code=%d, event_time=%" PRId64,
                                  keysym, xkev.keycode-8, event_time);
#endif

                    if (xkev.type == KeyPress)
                        core_keyboard->key_press(event_time, keysym, xkev.keycode - 8);
                    else
                        core_keyboard->key_release(event_time, keysym, xkev.keycode - 8);

                    break;
                }

            case ButtonPress:
            case ButtonRelease:
                {
                    auto const& xbev = xev.xbutton;
                    auto const up = Button4, down = Button5, left = 6, right = 7;

#ifdef MIR_ON_X11_INPUT_VERBOSE
                    mir::log_info("X11 button event :"
                                  " type=%s, serial=%u, send_event=%d, display=%p, window=%p,"
                                  " root=%p, subwindow=%p, time=%d, x=%d, y=%d, x_root=%d,"
                                  " y_root=%d, state=0x%0X, button=%d, same_screen=%d",
                                  xbev.type == ButtonPress ? "down" : "up", xbev.serial,
                                  xbev.send_event, xbev.display, xbev.window, xbev.root,
                                  xbev.subwindow, xbev.time, xbev.x, xbev.y, xbev.x_root,
                                  xbev.y_root, xbev.state, xbev.button, xbev.same_screen);
#endif
                    if (xbev.type == ButtonRelease && xbev.button >= up && xbev.button <= right)
                    {
#ifdef MIR_ON_X11_INPUT_VERBOSE
                        mir::log_info("Swallowed");
#endif
                        break;
                    }
                    auto const event_time =
                        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds{xbev.time});
                    auto pos = get_pos_on_output(xbev.window, xbev.x, xbev.y);
                    core_pointer->update_button_state(xbev.state);

                    if (xbev.button >= up && xbev.button <= right)
                    {  // scroll event
                        core_pointer->pointer_motion(
                            event_time,
                            pos,
                            geom::Displacement{xbev.button == right ? 1 : xbev.button == left ? -1 : 0,
                                               xbev.button == up ? 1 : xbev.button == down ? -1 : 0});
                    }
                    else
                    {
                        if (xbev.type == ButtonPress)
                            core_pointer->pointer_press(
                                event_time,
                                xbev.button,
                                pos,
                                geom::Displacement{0, 0});
                        else
                            core_pointer->pointer_release(
                                event_time,
                                xbev.button,
                                pos,
                                geom::Displacement{0, 0});
                    }
                    break;
                }

            case MotionNotify:
                {
                    auto const& xmev = xev.xmotion;

#ifdef MIR_ON_X11_INPUT_VERBOSE
                    mir::log_info("X11 motion event :"
                                  " type=motion, serial=%u, send_event=%d, display=%p, window=%p,"
                                  " root=%p, subwindow=%p, time=%d, x=%d, y=%d, x_root=%d,"
                                  " y_root=%d, state=0x%0X, is_hint=%s, same_screen=%d",
                                  xmev.serial, xmev.send_event, xmev.display, xmev.window,
                                  xmev.root, xmev.subwindow, xmev.time, xmev.x, xmev.y, xmev.x_root,
                                  xmev.y_root, xmev.state, xmev.is_hint == NotifyNormal ? "no" : "yes", xmev.same_screen);
#endif

                    core_pointer->update_button_state(xmev.state);
                    auto pos = get_pos_on_output(xmev.window, xmev.x, xmev.y);
                    core_pointer->pointer_motion(
                        std::chrono::milliseconds{xmev.time}, pos, geom::Displacement{0, 0});

                    break;
                }

            case ConfigureNotify:
                {
#ifdef MIR_ON_X11_INPUT_VERBOSE
                    auto const& xcev = xev.xconfigure;
                    mir::log_info("Window size : %dx%d", xcev.width, xcev.height);
#endif
                    break;
                }

            case MappingNotify:
#ifdef MIR_ON_X11_INPUT_VERBOSE
                mir::log_info("Keyboard mapping changed at server. Refreshing the cache.");
#endif
                XRefreshKeyboardMapping(&(xev.xmapping));
                break;

            case ClientMessage:
                mir::log_info("Exiting");
                kill(getpid(), SIGTERM);
                break;

            default:
#ifdef MIR_ON_X11_INPUT_VERBOSE
                mir::log_info("Uninteresting event : %08X", xev.type);
#endif
                break;
            }
        }
        else
            mir::log_error("input event received with no sink to handle it");
    }
}
