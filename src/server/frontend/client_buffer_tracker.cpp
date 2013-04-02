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
	: ids()
{
}

void mf::ClientBufferTracker::add(mc::BufferID const& id)
{
	auto existing_id = find_buffer(id);

	if (existing_id != ids.end())
	{
		ids.push_front(*existing_id);
		ids.erase(existing_id);
	}
	else
	{
		ids.push_front(id.as_uint32_t());
	}
	if (ids.size() > 3)
		ids.pop_back();
}

bool mf::ClientBufferTracker::client_has(mc::BufferID const& id)
{
	return find_buffer(id) != ids.end();
}

std::list<uint32_t>::iterator mf::ClientBufferTracker::find_buffer(compositor::BufferID const& id)
{
	auto iterator = ids.begin();

	for (; iterator != ids.end() && *iterator != id.as_uint32_t(); ++iterator);

	return iterator;
}
