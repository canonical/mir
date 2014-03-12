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
 */

#include "mir_toolkit/mir_trust_session.h"
#include "mir_trust_session.h"
#include "mir_connection.h"

#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace
{

// assign_result is compatible with all 2-parameter callbacks
void assign_result(void *result, void **context)
{
    if (context)
        *context = result;
}

}

MirTrustSession* mir_connection_create_trust_session(MirConnection* connection)
{
    return connection->create_trust_session();
}

MirTrustSessionAddTrustResult mir_trust_session_add_trusted_pid(MirTrustSession *trust_session,
                                                                pid_t pid)
{
    return trust_session->add_trusted_pid(pid);
}

MirWaitHandle *mir_trust_session_start(MirTrustSession *trust_session,
                                       mir_trust_session_callback callback,
                                       void* context)
{
    try
    {
        return trust_session->start(callback, context);
    }
    catch (std::exception const&)
    {
        // TODO callback with an error
        return nullptr;
    }
}

MirBool mir_trust_session_start_sync(MirTrustSession *trust_session)
{
    mir_wait_for(mir_trust_session_start(trust_session,
        reinterpret_cast<mir_trust_session_callback>(assign_result),
        NULL));
    return trust_session->get_state() == mir_trust_session_state_started ? mir_true : mir_false;
}

MirWaitHandle *mir_trust_session_stop(MirTrustSession *trust_session,
                                      mir_trust_session_callback callback,
                                      void* context)
{
    try
    {
        return trust_session->stop(callback, context);
    }
    catch (std::exception const&)
    {
        // TODO callback with an error
        return nullptr;
    }
}

MirBool mir_trust_session_stop_sync(MirTrustSession *trust_session)
{
    mir_wait_for(mir_trust_session_stop(trust_session,
        reinterpret_cast<mir_trust_session_callback>(assign_result),
        NULL));
    return trust_session->get_state() == mir_trust_session_state_stopped ? mir_true : mir_false;
}

MirTrustSessionState mir_trust_session_get_state(MirTrustSession *trust_session)
{
    return trust_session->get_state();
}

void mir_trust_session_set_event_callback(MirTrustSession* trust_session,
                                          mir_trust_session_event_callback callback,
                                          void* context)
{
    trust_session->register_trust_session_event_callback(callback, context);
}

void mir_trust_session_release(MirTrustSession* trust_session)
{
    delete trust_session;
}
