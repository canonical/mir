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

#ifndef MIR_PLATFORM_ANDROID_ANDROID_BUFFER_WRITER_H_
#define MIR_PLATFORM_ANDROID_ANDROID_BUFFER_WRITER_H_

#include "mir/graphics/buffer_writer.h"

#include <hardware/gralloc.h>

#include <memory>

namespace mir
{
namespace graphics
{
namespace android
{

class BufferWriter : public graphics::BufferWriter
{
public:
     BufferWriter();

     void write(graphics::Buffer& buffer, unsigned char const* pixels, size_t size) override;
private:
     gralloc_module_t const* hw_module;
};

}
}
}

#endif /* MIR_PLATFORM_ANDROID_ANDROID_BUFFER_WRITER_H_ */
