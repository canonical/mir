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

#include <shared_mutex>
#include <unordered_map>

class MirPresentationChain;
namespace mir
{
namespace client
{
class MirBuffer;
class PresentationChain;
class ConnectionSurfaceMap : public SurfaceMap
{
public:
    void with_surface_do(frontend::SurfaceId surface_id, std::function<void(MirSurface*)> const& exec) const override;
    void insert(frontend::SurfaceId surface_id, std::shared_ptr<MirSurface> const& surface);
    void erase(frontend::SurfaceId surface_id);

    void with_stream_do(frontend::BufferStreamId stream_id, std::function<void(ClientBufferStream*)> const& exec) const override;
    void with_all_streams_do(std::function<void(ClientBufferStream*)> const&) const override;

    void insert(frontend::BufferStreamId stream_id, std::shared_ptr<ClientBufferStream> const& chain);
    void insert(frontend::BufferStreamId stream_id, std::shared_ptr<MirPresentationChain> const& chain);
    void erase(frontend::BufferStreamId surface_id);

    //TODO: should have a mf::BufferID
    void insert(int buffer_id, std::shared_ptr<MirBuffer> const& buffer) override;
    void erase(int buffer_id) override;
    std::shared_ptr<MirBuffer> buffer(int buffer_id) const override;

private:
    std::shared_timed_mutex mutable guard;
    std::unordered_map<frontend::SurfaceId, std::shared_ptr<MirSurface>> surfaces;
    std::shared_timed_mutex mutable stream_guard;
    std::unordered_map<frontend::BufferStreamId, std::shared_ptr<ClientBufferStream>> streams;
    std::unordered_map<frontend::BufferStreamId, std::shared_ptr<MirPresentationChain>> chains;
    std::shared_timed_mutex mutable buffer_guard;
    std::unordered_map<int, std::shared_ptr<MirBuffer>> buffers;
};

}
}
#endif /* MIR_CLIENT_CONNECTION_SURFACE_MAP_H_ */
