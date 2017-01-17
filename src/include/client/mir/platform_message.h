/*
 * Copyright © 2017 Canonical Ltd.
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
 */

#ifndef MIR_PLATFORM_PLATFORM_MESSAGE_H_
#define MIR_PLATFORM_PLATFORM_MESSAGE_H_

#include <vector>

struct MirPlatformMessage
{
    MirPlatformMessage(unsigned int opcode) : opcode{opcode} {}

    unsigned int const opcode;
    std::vector<char> data;
    std::vector<int> fds;
};

#endif // MIR_PLATFORM_PLATFORM_MESSAGE_H_
