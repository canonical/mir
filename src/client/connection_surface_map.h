/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_CONNECTION_SURFACE_MAP_H_
#define MIR_CLIENT_CONNECTION_SURFACE_MAP_H_

#include "surface_map.h"

#include <unordered_map>
#include <mutex>

namespace mir
{
namespace client
{

class ConnectionSurfaceMap : public SurfaceMap
{
public:
    ConnectionSurfaceMap();

    void with_surface_do(int surface_id, std::function<void(MirSurface*)> exec) const override;
    void insert(int surface_id, MirSurface* surface);
    void erase(int surface_id);

private:
    std::mutex mutable guard;
    std::unordered_map<int, MirSurface*> surfaces;
};

}
}
#endif /* MIR_CLIENT_CONNECTION_SURFACE_MAP_H_ */
