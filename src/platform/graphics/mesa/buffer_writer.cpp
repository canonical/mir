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

#include "buffer_writer.h"

#include "shm_buffer.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;

mgm::BufferWriter::BufferWriter()
{
}

void mgm::BufferWriter::write(mg::Buffer& buffer, unsigned char const* data, size_t size)
{
    auto shm_buffer = dynamic_cast<mgm::ShmBuffer*>(&buffer);
    if (!shm_buffer)
        BOOST_THROW_EXCEPTION(std::logic_error("Direct CPU write is only supported to software allocated buffers on mesa platform"));
    
    shm_buffer->write(data, size);
}
