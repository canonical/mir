/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
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

class mfd::BinderSession : public MessageSender
{
public:

    BinderSession();

    void set_processor(std::shared_ptr<MessageProcessor> const& processor);

    bool process_message(android::Parcel const& request, android::Parcel* response)
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

private:
    void send(const std::ostringstream& buffer2);
    void send_fds(std::vector<int32_t> const& fd);

    std::shared_ptr<MessageProcessor> processor;

    android::Parcel* response;
};

mfd::BinderSession::BinderSession() :
    processor(std::make_shared<NullMessageProcessor>()),
    response(0)
{
}

void mfd::BinderSession::set_processor(std::shared_ptr<MessageProcessor> const& processor)
{
    this->processor = processor;
}

void mfd::BinderSession::send(const std::ostringstream& buffer)
{
    assert(response);

    auto const& as_str(buffer.str());

    response->writeString8(android::String8(as_str.data(), as_str.length()));
}

void mfd::BinderSession::send_fds(std::vector<int32_t> const& fds)
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
    this->ipc_factory = ipc_factory;
    sessions.clear();
}

android::status_t mfd::BinderService::onTransact(
    uint32_t /*code*/,
    const android::Parcel& request,
    android::Parcel* response,
    uint32_t /*flags*/)
{
    auto const client_pid = android::IPCThreadState::self()->getCallingPid();
    auto& session = sessions[client_pid];

    if (!session)
    {
        session = std::make_shared<BinderSession>();

        session->set_processor(
            std::make_shared<detail::ProtobufMessageProcessor>(
                session.get(),
                ipc_factory->make_ipc_server(),
                ipc_factory->resource_cache()));
    }

    if (!session->process_message(request, response))
        sessions.erase(client_pid);

    return android::OK;
}
