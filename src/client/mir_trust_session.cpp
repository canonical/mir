/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "mir_trust_session.h"
#include "event_handler_register.h"

namespace mp = mir::protobuf;
namespace mcl = mir::client;

MirTrustSession::MirTrustSession(
    mp::DisplayServer& server,
    std::shared_ptr<mcl::EventHandlerRegister> const& event_handler_register) :
    server(server),
    event_handler_register(event_handler_register),
    event_handler_register_id{event_handler_register->register_event_handler(
        [this](MirEvent const& event)
        {   if (event.type == mir_event_type_trust_session_state_change)
                set_state(event.trust_session.new_state);
        })},
    state(mir_trust_session_state_stopped),
    handle_trust_session_event{[](MirTrustSessionState){}}
{
}

MirTrustSession::~MirTrustSession()
{
    set_state(mir_trust_session_state_stopped);
}

MirTrustSessionState MirTrustSession::get_state() const
{
    std::lock_guard<decltype(event_handler_mutex)> lock(event_handler_mutex);
    return state;
}

void MirTrustSession::set_state(MirTrustSessionState new_state)
{
    std::lock_guard<decltype(event_handler_mutex)> lock(event_handler_mutex);

    if (new_state != state)
    {
        handle_trust_session_event(new_state);

        if (new_state == mir_trust_session_state_stopped)
        {
            event_handler_register->unregister_event_handler(event_handler_register_id);
        }

        state = new_state;
    }
}

MirWaitHandle* MirTrustSession::start(pid_t pid, mir_trust_session_callback callback, void* context)
{
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        parameters.mutable_base_trusted_session()->set_pid(pid);
        start_wait_handle.expect_result();
    }

    server.start_trust_session(
        0,
        &parameters,
        &session,
        google::protobuf::NewCallback(this, &MirTrustSession::done_start,
                                      callback, context));

    return &start_wait_handle;
}

MirWaitHandle* MirTrustSession::stop(mir_trust_session_callback callback, void* context)
{
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        stop_wait_handle.expect_result();
    }

    server.stop_trust_session(
        0,
        &protobuf_void,
        &protobuf_void,
        google::protobuf::NewCallback(this, &MirTrustSession::done_stop,
                                      callback, context));

    return &stop_wait_handle;
}

MirWaitHandle* MirTrustSession::add_trusted_session(pid_t pid,
    mir_trust_session_add_trusted_session_callback callback,
    void* context)
{
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        trusted_session.set_pid(pid);
        add_result_wait_handle.expect_result();
    }

    server.add_trusted_session(
        0,
        &trusted_session,
        &add_result,
        google::protobuf::NewCallback(this, &MirTrustSession::done_add_trusted_session,
                                      callback, context));

    return &add_result_wait_handle;
}

void MirTrustSession::register_trust_session_event_callback(
    mir_trust_session_event_callback callback,
    void* context)
{
    std::lock_guard<decltype(event_handler_mutex)> lock(event_handler_mutex);

    handle_trust_session_event =
        [this, callback, context](MirTrustSessionState new_state)
        {
            callback(this, new_state, context);
        };
}

void MirTrustSession::done_start(mir_trust_session_callback callback, void* context)
{
    {
        std::lock_guard<decltype(session_mutex)> lock(session_mutex);

        state = session.has_error() ? mir_trust_session_state_stopped : mir_trust_session_state_started;
    }

    callback(this, context);
    start_wait_handle.result_received();
}

void MirTrustSession::done_stop(mir_trust_session_callback callback, void* context)
{
    set_state(mir_trust_session_state_stopped);

    callback(this, context);
    stop_wait_handle.result_received();
}

void MirTrustSession::done_add_trusted_session(mir_trust_session_add_trusted_session_callback callback, void* context)
{
    MirBool added = mir_true;
    if (add_result.has_error())
    {
        added = mir_false;
    }
    callback(this, added, context);
    add_result_wait_handle.result_received();
}

char const* MirTrustSession::get_error_message()
{
    std::lock_guard<decltype(session_mutex)> lock(session_mutex);

    if (!session.has_error())
        session.set_error(std::string{});

    return session.error().c_str();
}

MirWaitHandle* MirTrustSession::new_fds_for_prompt_providers(
    unsigned int no_of_fds,
    mir_client_fd_callback callback,
    void * context)
{
    mir::protobuf::SocketFDRequest request;
    request.set_number(no_of_fds);

    server.new_fds_for_prompt_providers(
        nullptr,
        &request,
        &socket_fd_response,
        google::protobuf::NewCallback(this, &MirTrustSession::done_fds_for_trusted_clients,
                                              callback, context));

    return &fds_for_trusted_clients_wait_handle;
}

void MirTrustSession::done_fds_for_trusted_clients(
    mir_client_fd_callback callback,
    void* context)
{
    auto const size = socket_fd_response.fd_size();

    std::vector<int> fds;
    fds.reserve(size);

    for (auto i = 0; i != size; ++i)
        fds.push_back(socket_fd_response.fd(i));

    callback(this, size, fds.data(), context);
    fds_for_trusted_clients_wait_handle.result_received();
}
