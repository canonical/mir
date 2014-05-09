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
 * Authored by: Nick Dedekind <nick.dedekind@gmail.com>
 */

#include "mir_trust_session.h"
#include "event_distributor.h"

namespace mp = mir::protobuf;
namespace mcl = mir::client;

MirTrustSession::MirTrustSession(
    mp::DisplayServer& server,
    std::shared_ptr<mcl::EventDistributor> const& event_distributor)
    : server(server),
      event_distributor(event_distributor),
      state(mir_trust_session_state_stopped)
{
    event_distributor_fn_id = event_distributor->register_event_handler(
        [this](MirEvent const& event)
        {
            if (event.type != mir_event_type_trust_session_state_change)
                return;

            std::lock_guard<decltype(mutex_event_handler)> lock(mutex_event_handler);

            set_state(event.trust_session.new_state);
            if (handle_trust_session_event) {
                handle_trust_session_event(event.trust_session.new_state);
            }
        }
    );
}

MirTrustSession::~MirTrustSession()
{
    event_distributor->unregister_event_handler(event_distributor_fn_id);
}

MirTrustSessionState MirTrustSession::get_state() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return state;
}

void MirTrustSession::set_state(MirTrustSessionState new_state)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    state = new_state;
}

MirWaitHandle* MirTrustSession::start(pid_t pid, mir_trust_session_callback callback, void* context)
{
    mir::protobuf::TrustSessionParameters parameters;
    parameters.mutable_base_trusted_session()->set_pid(pid);

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
    mir::protobuf::TrustedSession trusted_session;
    trusted_session.set_pid(pid);

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
    std::lock_guard<decltype(mutex_event_handler)> lock(mutex_event_handler);

    handle_trust_session_event =
        [this, callback, context](MirTrustSessionState new_state)
        {
            callback(this, new_state, context);
        };
}

void MirTrustSession::done_start(mir_trust_session_callback callback, void* context)
{
    std::string error;
    MirTrustSessionState new_state = mir_trust_session_state_stopped;
    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        if (session.has_state())
            new_state = (MirTrustSessionState)session.state();

        error = session.error();
    }
    set_error_message(error);
    set_state(new_state);

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
    MirTrustSessionAddTrustResult result = static_cast<MirTrustSessionAddTrustResult>(add_result.result());
    if (add_result.has_error())
    {
        set_error_message(add_result.error());
        result = mir_trust_session_add_tust_failed;
    }
    callback(this, result, context);
    add_result_wait_handle.result_received();
}

char const* MirTrustSession::get_error_message()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (session.has_error())
    {
        return session.error().c_str();
    }
    else
    {
        return error_message.c_str();
    }
}

void MirTrustSession::set_error_message(std::string const& error)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    error_message = error;
}
