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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/frontend/client_buffer_tracker.h"
#include "mir/compositor/buffer_id.h"

namespace mf = mir::frontend;
namespace mc = mir::compositor;

mf::ClientBufferTracker::ClientBufferTracker()
	: id_age_map()
{
}

void mf::ClientBufferTracker::add(mc::BufferID const& id)
{
	id_age_map[id.as_uint32_t()] = 0;
}

bool mf::ClientBufferTracker::client_has(mc::BufferID const& id)
{
	return id_age_map.find(id.as_uint32_t()) != id_age_map.end();
}
