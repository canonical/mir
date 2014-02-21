/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "mir_trusted_prompt_session.h"

namespace mp = mir::protobuf;

MirTrustedPromptSession::MirTrustedPromptSession(
    MirConnection *allocating_connection,
    mp::DisplayServer::Stub & server)
    : server(server),
      connection(allocating_connection)
{
}

MirTrustedPromptSession::~MirTrustedPromptSession()
{
}

MirTrustedPromptSessionAddApplicationResult MirTrustedPromptSession::add_app_with_pid(pid_t pid)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    auto app = parameters.add_application();
    app->set_pid(pid);

    return mir_trusted_prompt_session_app_addition_succeeded;
}

MirWaitHandle* MirTrustedPromptSession::start(mir_tps_callback callback, void * context)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    server.start_trusted_session(
        0,
        &parameters,
        &session,
        google::protobuf::NewCallback(this, &MirTrustedPromptSession::done_start,
                                      callback, context));

    return &start_wait_handle;
}

MirWaitHandle* MirTrustedPromptSession::stop(mir_tps_callback callback, void * context)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    server.stop_trusted_session(
        0,
        &session.id(),
        &protobuf_void,
        google::protobuf::NewCallback(this, &MirTrustedPromptSession::done_stop,
                                      callback, context));

    return &stop_wait_handle;
}

void MirTrustedPromptSession::done_start(mir_tps_callback callback, void* context)
{
    set_error_message(session.error());

    callback(this, context);
    start_wait_handle.result_received();
}

void MirTrustedPromptSession::done_stop(mir_tps_callback callback, void* context)
{
    callback(this, context);
    stop_wait_handle.result_received();
}

char const * MirTrustedPromptSession::get_error_message()
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

void MirTrustedPromptSession::set_error_message(std::string const& error)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    error_message = error;
}

int MirTrustedPromptSession::id() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return session.id().value();
}