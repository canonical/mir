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

#include <epoxy/egl.h>

#include <functional>
#include <utility>

// EGLStream uses Epoxy's dispatch slots, while MockEGL implements the plain EGL symbols.
auto redirect_epoxy_egl(
    decltype(epoxy_eglGetCurrentDisplay) current_display,
    decltype(epoxy_eglQueryString) query_string,
    decltype(epoxy_eglGetProcAddress) get_proc_address) -> std::function<void()>
{
    auto const old_current_display = std::exchange(epoxy_eglGetCurrentDisplay, current_display);
    auto const old_query_string = std::exchange(epoxy_eglQueryString, query_string);
    auto const old_get_proc_address = std::exchange(epoxy_eglGetProcAddress, get_proc_address);
    return [=]
    {
        epoxy_eglGetCurrentDisplay = old_current_display;
        epoxy_eglQueryString = old_query_string;
        epoxy_eglGetProcAddress = old_get_proc_address;
    };
}
