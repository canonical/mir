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

#include "rpc/mir_display_server.h"
#include "buffer_stream_configuration.h"

namespace mcl = mir::client;
namespace mclr = mir::client::rpc;

mcl::BufferStreamConfiguration::BufferStreamConfiguration(
    mclr::DisplayServer& server, frontend::BufferStreamId id)
    : server{server},
      id{id}
{
}

void mcl::BufferStreamConfiguration::on_swap_interval_set(int interval)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    swap_interval_ = interval;
    interval_wait_handle.result_received();
}

int mcl::BufferStreamConfiguration::swap_interval() const
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    return swap_interval_;
}

MirWaitHandle* mcl::BufferStreamConfiguration::set_swap_interval(int interval)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    if (interval == swap_interval_)
        return nullptr;
    lock.unlock();

    mir::protobuf::StreamConfiguration configuration;
    configuration.mutable_id()->set_value(id.as_value());
    configuration.set_swapinterval(interval);
    interval_wait_handle.expect_result();
    server.configure_buffer_stream(&configuration, protobuf_void.get(),
        google::protobuf::NewCallback(this, &mcl::BufferStreamConfiguration::on_swap_interval_set, interval));

    return &interval_wait_handle;
}

