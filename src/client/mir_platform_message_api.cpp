/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_platform_message.h"
#include "mir/platform_message.h"

MirPlatformMessage* mir_platform_message_create(unsigned int opcode)
{
    return new MirPlatformMessage{opcode};
}

void mir_platform_message_release(MirPlatformMessage const* message)
{
    delete message;
}

void mir_platform_message_set_data(MirPlatformMessage* message, void const* data, size_t data_size)
{
    auto const char_data = static_cast<char const*>(data);
    message->data.assign(char_data, char_data + data_size);
}

void mir_platform_message_set_fds(MirPlatformMessage* message, int const* fds, size_t num_fds)
{
    message->fds.assign(fds, fds + num_fds);
}

unsigned int mir_platform_message_get_opcode(MirPlatformMessage const* message)
{
    return message->opcode;
}

MirPlatformMessageData mir_platform_message_get_data(MirPlatformMessage const* message)
{
    return {message->data.data(), message->data.size()};
}

MirPlatformMessageFds mir_platform_message_get_fds(MirPlatformMessage const* message)
{
    return {message->fds.data(), message->fds.size()};
}
