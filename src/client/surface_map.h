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

#ifndef MIR_CLIENT_SURFACE_MAP_H_
#define MIR_CLIENT_SURFACE_MAP_H_

#include <mir_toolkit/client_types.h>
#include "mir/frontend/surface_id.h"
#include "mir/frontend/buffer_stream_id.h"
#include <functional>
#include <memory>

namespace mir
{
namespace protobuf
{
class Buffer;
}
namespace client
{
class MirBuffer;

class SurfaceMap
{
public:
    virtual std::shared_ptr<MirWindow> surface(frontend::SurfaceId) const = 0;
    virtual std::shared_ptr<MirBufferStream> stream(frontend::BufferStreamId) const = 0;
    virtual void with_all_streams_do(std::function<void(MirBufferStream*)> const&) const = 0;
    virtual std::shared_ptr<MirBuffer> buffer(int buffer_id) const = 0;
    virtual void insert(int buffer_id, std::shared_ptr<MirBuffer> const& buffer) = 0;
    virtual void erase(int buffer_id) = 0;  

protected:
    virtual ~SurfaceMap() = default;
    SurfaceMap() = default;
    SurfaceMap(const SurfaceMap&) = delete;
    SurfaceMap& operator=(const SurfaceMap&) = delete;
};

}
}
#endif /* MIR_CLIENT_SURFACE_MAP_H_ */
