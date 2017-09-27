/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "software_buffer.h"
#include "mir/shm_file.h"
#include "native_buffer.h"

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace mgc = mir::graphics::common;
namespace geom = mir::geometry;

mgm::SoftwareBuffer::SoftwareBuffer(
    std::unique_ptr<mir::ShmFile> shm_file,
    geom::Size const& size,
    MirPixelFormat const& pixel_format) :
    ShmBuffer(std::move(shm_file), size, pixel_format),
    native_buffer(create_native_buffer())
{
}

std::shared_ptr<mg::NativeBuffer> mgm::SoftwareBuffer::create_native_buffer()
{
    auto buffer = std::make_shared<mgm::NativeBuffer>();
    *static_cast<MirBufferPackage*>(buffer.get()) = *to_mir_buffer_package();
    buffer->bo = nullptr;
    return buffer;
}

std::shared_ptr<mg::NativeBuffer> mgm::SoftwareBuffer::native_buffer_handle() const
{
    return native_buffer;
}
