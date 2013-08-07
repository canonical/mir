/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/graphics/buffer_basic.h"
#include <atomic>

namespace mg = mir::graphics;

namespace
{
mg::BufferID generate_next_buffer_id()
{
    static std::atomic<uint32_t> next_id{0};

    auto id = mg::BufferID(next_id.fetch_add(1));

    // Avoid returning an "invalid" id. (Not sure we need invalid ids)
    while (!id.is_valid()) id = mg::BufferID(next_id.fetch_add(1));

    return id;
}
}

mg::BufferBasic::BufferBasic() :
    buffer_id(generate_next_buffer_id())
{
}
