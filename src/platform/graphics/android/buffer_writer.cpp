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
 * Authored by:
 *   Robert Carr <robert.carr@canonical.com>
 */

#include "buffer_writer.h"
#include "buffer.h"

#include "mir/graphics/android/android_native_buffer.h"

#include <hardware/hardware.h>

#include <boost/throw_exception.hpp>
#include <stdexcept>

#include <string.h>

namespace mg = mir::graphics;
namespace mga = mg::android;

mga::BufferWriter::BufferWriter()
{
    auto err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (hw_module_t const **)(&hw_module));
    if (err < 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("Could not open hardware module"));
}

void mga::BufferWriter::write(mg::Buffer& buffer, unsigned char const* data, size_t size)
{
    auto buffer_size = buffer.size();
    auto bpp = MIR_BYTES_PER_PIXEL(buffer.pixel_format());
    if (bpp * buffer_size.width.as_uint32_t() * buffer_size.height.as_uint32_t() != size)
        BOOST_THROW_EXCEPTION(std::logic_error("Size of pixels is not equal to size of buffer"));

    auto const& handle = buffer.native_buffer_handle();
    
    char* vaddr;
    int usage = GRALLOC_USAGE_SW_WRITE_OFTEN;
    int width = buffer.size().width.as_uint32_t();
    int height = buffer.size().height.as_uint32_t();
    int top = 0;
    int left = 0;
    if ( hw_module->lock(hw_module, handle->handle(),
        usage, top, left, width, height, reinterpret_cast<void**>(&vaddr)) )
        BOOST_THROW_EXCEPTION(std::runtime_error("error securing buffer for client cpu use"));

    // Copy line by line in case of stride != width*bpp
    for (int i = 0; i < height; i++)
    {
        int line_offset_in_buffer = buffer.stride().as_uint32_t()*i;
        int line_offset_in_source = bpp*width*i;
        memcpy(vaddr + line_offset_in_buffer, data + line_offset_in_source, width * bpp);
    }
    
    hw_module->unlock(hw_module, handle->handle());
}
