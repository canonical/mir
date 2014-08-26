/*
 * Copyright © 2014 Canonical Ltd.
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

#include "mir/graphics/platform_ipc_package.h"
#include "buffer_packer.h"

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;

void mga::BufferPacker::pack_buffer(BufferIpcMessage&, Buffer const&, BufferIpcMsgType) const
{
}

void mga::BufferPacker::unpack_buffer(BufferIpcMessage&, Buffer const&) const
{
}

std::shared_ptr<mg::PlatformIPCPackage> mga::BufferPacker::get_ipc_package()
{
    return std::make_shared<mg::PlatformIPCPackage>();
}
