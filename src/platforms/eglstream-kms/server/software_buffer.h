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

#ifndef MIR_PLATFORMS_EGLSTREAM_SOFTWARE_BUFFER_H_
#define MIR_PLATFORMS_EGLSTREAM_SOFTWARE_BUFFER_H_

#include "shm_buffer.h"

namespace mir
{
class ShmFile;

namespace graphics
{
namespace eglstream
{

class SoftwareBuffer: public common::FileBackedShmBuffer
{
public:
    SoftwareBuffer(
        std::unique_ptr<ShmFile> shm_file,
        geometry::Size const& size,
        MirPixelFormat const& pixel_format);

    std::shared_ptr<NativeBuffer> native_buffer_handle() const override;
private:
    std::shared_ptr<NativeBuffer> create_native_handle();
    std::shared_ptr<NativeBuffer> const native_handle;
};

}
}
}
#endif /* MIR_PLATFORMS_EGLSTREAM_SOFTWARE_BUFFER_H_ */
