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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include <limits>
#include "mir/compositor/buffer_id.h"

namespace mc=mir::compositor;

mc::BufferIDMonotonicIncreaseGenerator::BufferIDMonotonicIncreaseGenerator()
 : id_counter(0)
{}

mc::BufferID mc::BufferIDMonotonicIncreaseGenerator::generate_unique_id()
{
    if (id_counter == std::numeric_limits<uint32_t>::max() )
        return mc::BufferID{0};
    return mc::BufferID{++id_counter};
}
