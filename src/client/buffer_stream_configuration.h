/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_BUFFER_STREAM_CONFIGURATION_H_
#define MIR_CLIENT_BUFFER_STREAM_CONFIGURATION_H_

#include "mir_protobuf.pb.h"
#include "mir/frontend/buffer_stream_id.h"
#include "mir_wait_handle.h"
#include <mutex>

namespace mir
{
namespace client
{
namespace rpc { class DisplayServer; }
class BufferStreamConfiguration
{
public:
    BufferStreamConfiguration(rpc::DisplayServer& server, frontend::BufferStreamId id);

    void on_swap_interval_set(int interval);
    int swap_interval() const;
    MirWaitHandle* set_swap_interval(int interval);
private:
    rpc::DisplayServer& server;
    frontend::BufferStreamId id;
    std::unique_ptr<protobuf::Void> protobuf_void{std::make_unique<protobuf::Void>()};
    MirWaitHandle interval_wait_handle;
    std::mutex mutable mutex;
    int swap_interval_ = 1;
};

}
}
#endif /* MIR_CLIENT_BUFFER_STREAM_CONFIGURATION */
