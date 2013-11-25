/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "android_input_channel.h"

#include <androidfw/InputTransport.h>

#include <unistd.h>

namespace mia = mir::input::android;
namespace droidinput = android;

mia::AndroidInputChannel::AndroidInputChannel()
{

    droidinput::InputChannel::openInputFdPair(s_fd, c_fd);
}

mia::AndroidInputChannel::~AndroidInputChannel()
{
    close(s_fd);
    close(c_fd);
}

int mia::AndroidInputChannel::client_fd() const
{
    return c_fd;
}

int mia::AndroidInputChannel::server_fd() const
{
    return s_fd;
}
