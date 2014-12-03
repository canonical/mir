/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
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
#include <memory>
#include <vector>
#include <atomic>

struct MirPlatformMessage
{
    MirPlatformMessage(unsigned int opcode)
        : opcode{opcode}, use_count{1}
    {
    }

    MirPlatformMessage* ref() const
    {
        ++use_count;
        return const_cast<MirPlatformMessage*>(this);
    }

    void unref() const
    {
        if (--use_count == 0)
            delete this;
    }

    unsigned int const opcode;
    std::vector<int> data;
    mutable std::atomic<int> use_count;
};

MirPlatformMessage* mir_platform_message_create(unsigned int opcode)
{
    return new MirPlatformMessage{opcode};
}

MirPlatformMessage* mir_platform_message_ref(MirPlatformMessage const* message)
{
    return message->ref();
}

void mir_platform_message_unref(MirPlatformMessage const* message)
{
    message->unref();
}

void mir_platform_message_set_data(MirPlatformMessage* message, int const* data, size_t ndata)
{
    message->data.assign(data, data + ndata);
}

MirPlatformMessageData mir_platform_message_get_data(MirPlatformMessage const* message)
{
    return {message->data.data(), message->data.size()};
}

unsigned int mir_platform_message_get_opcode(MirPlatformMessage const* message)
{
    return message->opcode;
}
