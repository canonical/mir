/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "android_input_platform.h"

namespace mcl = mir::client;

mcl::AndroidInputPlatform::AndroidInputPlatform()
{
}

std::shared_ptr<mcl::InputReceiverThread> create_input_thread(int fd, std::function<void(MirEvent *)> callback)
{
    (void)fd;
    (void)callback;
    // TODO: ~Implement ~racarr
    return std::shared_ptr<mcl::InputReceiverThread>();
}
