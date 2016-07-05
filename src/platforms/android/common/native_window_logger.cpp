/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "native_window_logger.h"

namespace mga = mir::graphics::android;

void mga::ConsoleNativeWindowLogger::buffer_event(
    BufferEvent type, ANativeWindow* win, ANativeWindowBuffer* buf, int fence)
{
    (void) type; (void) win; (void) buf; (void) fence;
}

void mga::ConsoleNativeWindowLogger::buffer_event(
    BufferEvent type, ANativeWindow* win, ANativeWindowBuffer* buf)
{
    (void) type; (void) win; (void) buf;
}

void mga::ConsoleNativeWindowLogger::query_event(ANativeWindow* win, int type, int result)
{
    (void) win; (void) type; (void) result;
}

void mga::ConsoleNativeWindowLogger::perform_event(
    ANativeWindow* win, int type, std::vector<int> const& args)
{
    (void) win; (void) type; (void) args;
}
