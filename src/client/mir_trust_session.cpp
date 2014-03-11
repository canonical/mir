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
#include "trust_session_control.h"

namespace mp = mir::protobuf;
namespace mcl = mir::client;

MirTrustSession::MirTrustSession(
    mp::DisplayServer::Stub & server,
    std::shared_ptr<mcl::TrustSessionControl> const& trust_session_control)
    : server(server),
      trust_session_control(trust_session_control),
      state(mir_trust_session_stopped)
{
    trust_session_control_fn_id = trust_session_control->add_trust_session_event_handler(
        [this]
        (MirTrustSessionState new_state)
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);

            if (handle_trust_session_event) {
                handle_trust_session_event(new_state);
            }
            this->state.store(new_state);
        }
    );
}

MirTrustSession::~MirTrustSession()
{
    trust_session_control->remove_trust_session_event_handler(trust_session_control_fn_id);
}

MirTrustSessionState MirTrustSession::get_state() const
{
    return state.load();
}

MirTrustSessionAddTrustResult MirTrustSession::add_trusted_pid(pid_t pid)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    // check for duplicates
    for (auto i=0; i < parameters.application_size(); i++)
    {
        auto const& application = parameters.application(i);
        if (application.has_pid() && application.pid() == pid)
        {
            return mir_trust_session_pid_already_exists;
        }
    }

    auto app = parameters.add_application();
    app->set_pid(pid);

    return mir_trust_session_pid_added;
}

MirWaitHandle* MirTrustSession::start(mir_trust_session_callback callback, void * context)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    server.start_trust_session(
        0,
        &parameters,
        &session,
        google::protobuf::NewCallback(this, &MirTrustSession::done_start,
                                      callback, context));

    return &start_wait_handle;
}

MirWaitHandle* MirTrustSession::stop(mir_trust_session_callback callback, void * context)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    server.stop_trust_session(
        0,
        &protobuf_void,
        &protobuf_void,
        google::protobuf::NewCallback(this, &MirTrustSession::done_stop,
                                      callback, context));

    return &stop_wait_handle;
}

void MirTrustSession::register_trust_session_event_callback(mir_trust_session_event_callback callback, void* context)
{
    handle_trust_session_event =
        [this, callback, context]
        (MirTrustSessionState state)
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            callback(this, state, context);
        }
    ;
}

void MirTrustSession::done_start(mir_trust_session_callback callback, void* context)
{
    set_error_message(session.error());
    if (session.has_state())
        state.store((MirTrustSessionState)session.state());
    else
        state.store(mir_trust_session_stopped);

    callback(this, context);
    start_wait_handle.result_received();
}

void MirTrustSession::done_stop(mir_trust_session_callback callback, void* context)
{
    state.store(mir_trust_session_stopped);
    callback(this, context);
    stop_wait_handle.result_received();
}

char const * MirTrustSession::get_error_message()
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

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
    std::lock_guard<std::recursive_mutex> lock(mutex);

    error_message = error;
}
