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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;

/// An interface provided by the graphics platform which may be used on the server side
/// to write pixel data to buffers. Currently in use only by cursors and touchspots
/// (which can be seen as similar to inprocess-software-clients).
class BufferWriter
{
public:
    virtual ~BufferWriter() = default;

    virtual void write(std::shared_ptr<Buffer> const& buffer, unsigned char const* data, size_t size) = 0;

protected:
    BufferWriter() = default;
    BufferWriter(BufferWriter const&) = delete;
    BufferWriter& operator=(BufferWriter const&) = delete;
};

}
}
