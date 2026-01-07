/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_WAYLAND_SERVER_H
#define MIR_WAYLAND_SERVER_H

#include <cstdint>

struct wl_display;
struct wl_event_loop;
struct wl_event_source;

// wl_display
wl_display* wl_display_create();
void wl_display_destroy(wl_display* display);
wl_event_loop* wl_display_get_event_loop(wl_display* display);
void wl_display_add_socket(wl_display* display, const char* name);
void wl_display_terminate(wl_display* display);

// wl_event_loop
typedef int (*wl_event_loop_fd_func_t)(int fd, uint32_t mask, void *data);
wl_event_loop* wl_event_loop_create();
void wl_event_loop_destroy(wl_event_loop* loop);
wl_event_source* wl_event_loop_add_fd(
    wl_event_loop *loop,
    int fd,
    uint32_t mask,
    wl_event_loop_fd_func_t func,
    void *data);

#endif