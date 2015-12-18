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
    void with_surface_do(frontend::SurfaceId surface_id, std::function<void(MirSurface*)> const& exec) const override;
    void insert(frontend::SurfaceId surface_id, std::shared_ptr<MirSurface> const& surface);
    void erase(frontend::SurfaceId surface_id);

    void with_stream_do(frontend::BufferStreamId stream_id, std::function<void(ClientBufferStream*)> const& exec) const override;
    void with_all_streams_do(std::function<void(ClientBufferStream*)> const&) const override;
    void insert(frontend::BufferStreamId stream_id, std::shared_ptr<ClientBufferStream> const& stream);
    void erase(frontend::BufferStreamId surface_id);

private:
    std::mutex mutable guard;
    std::unordered_map<frontend::SurfaceId, std::shared_ptr<MirSurface>> surfaces;
    std::unordered_map<frontend::BufferStreamId, std::shared_ptr<ClientBufferStream>> streams;
};

}
}
#endif /* MIR_CLIENT_CONNECTION_SURFACE_MAP_H_ */
