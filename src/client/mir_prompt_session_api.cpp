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

#define MIR_LOG_COMPONENT "MirPromptSessionAPI"

#include "mir_toolkit/mir_prompt_session.h"
#include "mir_prompt_session.h"
#include "mir_connection.h"

#include "mir/uncaught.h"

#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace
{
void null_callback(MirPromptSession*, void*) {}
}

MirPromptSession *mir_connection_create_prompt_session_sync(
    MirConnection* connection,
    pid_t application_pid,
    mir_prompt_session_state_change_callback state_change_callback,
    void* context)
{
    try
    {
        auto prompt_session = connection->create_prompt_session();
        if (state_change_callback)
            prompt_session->register_prompt_session_state_change_callback(state_change_callback, context);

        mir_wait_for(prompt_session->start(application_pid,
                     null_callback,
                     nullptr));
        return prompt_session;
    }
    catch (std::exception const& ex)
    {
        // TODO callback with an error
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
        return nullptr;
    }
}

MirWaitHandle* mir_prompt_session_new_fds_for_prompt_providers(
    MirPromptSession *prompt_session,
    unsigned int no_of_fds,
    mir_client_fd_callback callback,
    void * context)
{
    try
    {
        return prompt_session ?
            prompt_session->new_fds_for_prompt_providers(no_of_fds, callback, context) :
            nullptr;
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
        return nullptr;
    }
}

void mir_prompt_session_release_sync(
    MirPromptSession *prompt_session)
{
    mir_wait_for(prompt_session->stop(&null_callback, nullptr));
    delete prompt_session;
}

bool mir_prompt_session_is_valid(MirPromptSession *prompt_session)
{
    auto const err = prompt_session->get_error_message();

    return !*err;
}

char const *mir_prompt_session_error_message(MirPromptSession *prompt_session)
{
    return prompt_session->get_error_message();
}
