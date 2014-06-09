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


#include "mir_toolkit/mir_trust_session.h"
#include "mir_trust_session.h"
#include "mir_connection.h"

#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace
{
void null_callback(MirTrustSession*, void*) {}

void add_trusted_session_callback(MirTrustSession*,
                                  MirBool added,
                                  void* context)
{
    if (context)
        *(MirBool*)context = added;
}

}

MirTrustSession *mir_connection_start_trust_session_sync(MirConnection* connection,
                                                         pid_t base_session_pid,
                                                         mir_trust_session_event_callback event_callback,
                                                         void* context)
{
    try
    {
        auto trust_session = connection->create_trust_session();
        if (event_callback)
            trust_session->register_trust_session_event_callback(event_callback, context);

        mir_wait_for(trust_session->start(base_session_pid,
                     null_callback,
                     nullptr));
        return trust_session;
    }
    catch (std::exception const&)
    {
        // TODO callback with an error
        return nullptr;
    }
}

MirWaitHandle *mir_trust_session_add_trusted_session(MirTrustSession *trust_session,
                                                     pid_t session_pid,
                                                     mir_trust_session_add_trusted_session_callback callback,
                                                     void* context)
{
    try
    {
        return trust_session->add_trusted_session(session_pid, callback, context);
    }
    catch (std::exception const&)
    {
        // TODO callback with an error
        return nullptr;
    }
}

MirBool mir_trust_session_add_trusted_session_sync(MirTrustSession *trust_session, pid_t base_session_pid)
{
    MirBool result;
    mir_wait_for(mir_trust_session_add_trusted_session(trust_session,
        base_session_pid,
        add_trusted_session_callback,
        &result));
    return result;
}

MirWaitHandle* mir_trust_session_new_fds_for_prompt_providers(
    MirTrustSession *trust_session,
    unsigned int no_of_fds,
    mir_client_fd_callback callback,
    void * context)
{
    try
    {
        return trust_session ?
            trust_session->new_fds_for_prompt_providers(no_of_fds, callback, context) :
            nullptr;
    }
    catch (std::exception const&)
    {
        return nullptr;
    }
}

void mir_trust_session_release_sync(MirTrustSession *trust_session)
{
    mir_wait_for(trust_session->stop(&null_callback, nullptr));
    delete trust_session;
}

MirTrustSessionState mir_trust_session_get_state(MirTrustSession *trust_session)
{
    return trust_session->get_state();
}
