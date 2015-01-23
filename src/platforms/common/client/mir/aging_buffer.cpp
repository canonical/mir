/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "aging_buffer.h"

namespace mcl = mir::client;

mcl::AgingBuffer::AgingBuffer()
    : buffer_age(0)
{
}

uint32_t mcl::AgingBuffer::age() const
{
    return buffer_age;
}

void mcl::AgingBuffer::increment_age()
{
    if (buffer_age != 0)
        ++buffer_age;
}

void mcl::AgingBuffer::mark_as_submitted()
{
    buffer_age = 1;
}
