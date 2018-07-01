/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_PLATFORM_OPERATION_MESSAGE_H_
#define MIR_GRAPHICS_PLATFORM_OPERATION_MESSAGE_H_

#include <vector>
#include <cinttypes>
#include "mir/fd.h"

namespace mir
{
namespace graphics
{

struct PlatformOperationMessage
{
    std::vector<uint8_t> data;
    std::vector<mir::Fd> fds;
};

}
}

#endif
