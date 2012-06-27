/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_COMPOSITOR_BUFFER_H_
#define MIR_COMPOSITOR_BUFFER_H_

#include <cstdint>

namespace mir
{
namespace compositor
{

enum class PixelFormat : uint32_t {
    rgba_8888
};

class Buffer
{
 public:

    virtual uint32_t width() const = 0;

    virtual uint32_t height() const = 0;

    virtual uint32_t stride() const = 0;

    virtual PixelFormat pixel_format() const = 0;

 protected:
    Buffer() = default;
    ~Buffer() = default;
    Buffer(Buffer const&) = delete;
    Buffer& operator=(Buffer const&) = delete;
};

}
}
#endif // MIR_COMPOSITOR_BUFFER_H_
