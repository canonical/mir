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
      trust_session_control_fn_id(-1)
{
}

MirTrustSession::~MirTrustSession()
{
    if (trust_session_control_fn_id == -1)
    {
        trust_session_control->remove_trust_session_event_handler(trust_session_control_fn_id);
    }
}

MirTrustSessionAddApplicationResult MirTrustSession::add_app_with_pid(pid_t pid)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    // check for duplicates
    for (auto i=0; i < parameters.application_size(); i++)
    {
        auto const& application = parameters.application(i);
        if (application.has_pid() && application.pid() == pid)
        {
            return mir_trust_session_app_already_part_of_trust_session;
        }
    }

    auto app = parameters.add_application();
    app->set_pid(pid);

    return mir_trust_session_app_addition_succeeded;
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
        &session.id(),
        &protobuf_void,
        google::protobuf::NewCallback(this, &MirTrustSession::done_stop,
                                      callback, context));

    return &stop_wait_handle;
}

void MirTrustSession::register_trust_session_event_callback(mir_trust_session_event_callback callback, void* context)
{
    if (trust_session_control_fn_id != -1)
    {
        trust_session_control->remove_trust_session_event_handler(trust_session_control_fn_id);
    }

    trust_session_control_fn_id = trust_session_control->add_trust_session_event_handler(
        [this, callback, context]
        (int id, MirTrustSessionState state)
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);

            if (session.id().value() == id) {
                callback(this, state, context);
            }
        }
    );
}

void MirTrustSession::done_start(mir_trust_session_callback callback, void* context)
{
    set_error_message(session.error());

    callback(this, context);
    start_wait_handle.result_received();
}

void MirTrustSession::done_stop(mir_trust_session_callback callback, void* context)
{
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

int MirTrustSession::id() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return session.id().value();
}