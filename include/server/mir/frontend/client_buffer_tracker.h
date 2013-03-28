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

 #ifndef MIR_FRONTEND_CLIENT_BUFFER_TRACKER_H_
 #define MIR_FRONTEND_CLIENT_BUFFER_TRACKER_H_

#include <stdint.h>
#include <map>

namespace mir
{

namespace compositor
{
class BufferID;
}

namespace frontend
{

class ClientBufferTracker
{
public:
	ClientBufferTracker();

	void add(compositor::BufferID const& id);
	bool client_has(compositor::BufferID const& id);
private:
	std::map<uint32_t, uint32_t> id_age_map;
};

}
}

 #endif // MIR_FRONTEND_CLIENT_BUFFER_TRACKER_H_