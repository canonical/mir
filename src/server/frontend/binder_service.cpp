/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "binder_service.h"
#include "protobuf_message_processor.h"

#include "mir/frontend/protobuf_ipc_factory.h"

#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <utils/String8.h>

#include <sstream>
#include <stdexcept>

namespace mfd = mir::frontend::detail;

class mfd::BinderCallContext : public MessageSender
{
public:

    BinderCallContext(
        std::shared_ptr<protobuf::DisplayServer> const& display_server,
        std::shared_ptr<ResourceCache> const& resource_cache);

    bool process_message(android::Parcel const& request, android::Parcel* response);

private:
    void send(const std::ostringstream& buffer2);
    void send_fds(std::vector<int32_t> const& fd);

    std::shared_ptr<MessageProcessor> const processor;

    android::Parcel* response;
};

mfd::BinderCallContext::BinderCallContext(
    std::shared_ptr<protobuf::DisplayServer> const& mediator,
    std::shared_ptr<ResourceCache> const& resource_cache) :
    processor(std::make_shared<detail::ProtobufMessageProcessor>(
            this,
            mediator,
            resource_cache)),
    response(0)
{
}

bool mfd::BinderCallContext::process_message(android::Parcel const& request, android::Parcel* response)
{
    this->response = response;

    // TODO there's probably a way to refactor this to avoid copy
    auto const& buffer = request.readString8();
    std::string inefficient_copy(buffer.string(), buffer.string()+buffer.size());
    std::istringstream msg(inefficient_copy);

    auto const result = processor->process_message(msg);

    assert(response == this->response);
    this->response = 0;

    return result;
}


void mfd::BinderCallContext::send(const std::ostringstream& buffer)
{
    assert(response);

    auto const& as_str(buffer.str());

    response->writeString8(android::String8(as_str.data(), as_str.length()));
}

void mfd::BinderCallContext::send_fds(std::vector<int32_t> const& fds)
{
    assert(response);

    for (auto fd: fds)
        response->writeFileDescriptor(fd, true);
}

mfd::BinderService::BinderService()
{
}

mfd::BinderService::~BinderService()
{
}

void mfd::BinderService::set_ipc_factory(std::shared_ptr<ProtobufIpcFactory> const& ipc_factory)
{
    std::lock_guard<std::mutex> lock(guard);
    this->ipc_factory = ipc_factory;
    sessions.clear();
}

std::shared_ptr<mir::protobuf::DisplayServer> mfd::BinderService::get_session_for(pid_t client_pid)
{
    std::lock_guard<std::mutex> lock(guard);
    auto& session = sessions[client_pid];
    if (!session)
    {
        session = ipc_factory->make_ipc_server();
    }

    return session;
}

void mfd::BinderService::close_session_for(pid_t client_pid)
{
    std::lock_guard<std::mutex> lock(guard);
    sessions.erase(client_pid);
}

android::status_t mfd::BinderService::onTransact(
    uint32_t /*code*/,
    const android::Parcel& request,
    android::Parcel* response,
    uint32_t /*flags*/)
{
    auto const client_pid = android::IPCThreadState::self()->getCallingPid();

    BinderCallContext context(get_session_for(client_pid), ipc_factory->resource_cache());

    if (!context.process_message(request, response))
    {
        close_session_for(client_pid);
    }

    return android::OK;
}
