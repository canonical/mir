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

#include "mir_trusted_session.h"
#include "trusted_session_control.h"

namespace mp = mir::protobuf;
namespace mcl = mir::client;

MirTrustedSession::MirTrustedSession(
    mp::DisplayServer::Stub & server,
    std::shared_ptr<mcl::TrustedSessionControl> const& trusted_session_control)
    : server(server),
      trusted_session_control(trusted_session_control),
      trusted_session_control_fn_id(-1)
{
}

MirTrustedSession::~MirTrustedSession()
{
    if (trusted_session_control_fn_id == -1)
    {
        trusted_session_control->remove_trusted_session_event_handler(trusted_session_control_fn_id);
    }
}

MirTrustedSessionAddApplicationResult MirTrustedSession::add_app_with_pid(pid_t pid)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    // check for duplicates
    for (auto i=0; i < parameters.application_size(); i++)
    {
        auto const& application = parameters.application(i);
        if (application.has_pid() && application.pid() == pid)
        {
            return mir_trusted_session_app_already_part_of_trusted_session;
        }
    }

    auto app = parameters.add_application();
    app->set_pid(pid);

    return mir_trusted_session_app_addition_succeeded;
}

MirWaitHandle* MirTrustedSession::start(mir_trusted_session_callback callback, void * context)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    server.start_trusted_session(
        0,
        &parameters,
        &session,
        google::protobuf::NewCallback(this, &MirTrustedSession::done_start,
                                      callback, context));

    return &start_wait_handle;
}

MirWaitHandle* MirTrustedSession::stop(mir_trusted_session_callback callback, void * context)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    server.stop_trusted_session(
        0,
        &session.id(),
        &protobuf_void,
        google::protobuf::NewCallback(this, &MirTrustedSession::done_stop,
                                      callback, context));

    return &stop_wait_handle;
}

void MirTrustedSession::register_trusted_session_event_callback(mir_trusted_session_event_callback callback, void* context)
{
    if (trusted_session_control_fn_id != -1)
    {
        trusted_session_control->remove_trusted_session_event_handler(trusted_session_control_fn_id);
    }

    trusted_session_control_fn_id = trusted_session_control->add_trusted_session_event_handler(
        [this, callback, context]
        (int id, MirTrustedSessionState state)
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);

            if (session.id().value() == id) {
                callback(this, state, context);
            }
        }
    );
}

void MirTrustedSession::done_start(mir_trusted_session_callback callback, void* context)
{
    set_error_message(session.error());

    callback(this, context);
    start_wait_handle.result_received();
}

void MirTrustedSession::done_stop(mir_trusted_session_callback callback, void* context)
{
    callback(this, context);
    stop_wait_handle.result_received();
}

char const * MirTrustedSession::get_error_message()
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

void MirTrustedSession::set_error_message(std::string const& error)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    error_message = error;
}

int MirTrustedSession::id() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return session.id().value();
}