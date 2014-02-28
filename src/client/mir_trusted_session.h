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

#ifndef MIR_CLIENT_MIR_TRUSTED_SESSION_H_
#define MIR_CLIENT_MIR_TRUSTED_SESSION_H_

#include "mir_protobuf.pb.h"
#include "mir_wait_handle.h"
#include "client_trusted_session.h"

#include <mutex>
#include <memory>

namespace mir
{
/// The client-side library implementation namespace
namespace client
{
class TrustedSessionControl;
}
}

struct MirTrustedSession : public mir::client::TrustedSession
{
public:
    MirTrustedSession(mir::protobuf::DisplayServer::Stub & server,
                      std::shared_ptr<mir::client::TrustedSessionControl> const& trusted_session_control);

    ~MirTrustedSession();

    MirTrustedSession(MirTrustedSession const &) = delete;
    MirTrustedSession& operator=(MirTrustedSession const &) = delete;

    MirTrustedSessionAddApplicationResult add_app_with_pid(pid_t pid);

    MirWaitHandle* start(mir_trusted_session_callback callback, void * context);
    MirWaitHandle* stop(mir_trusted_session_callback callback, void * context);

    void register_trusted_session_event_callback(mir_trusted_session_event_callback callback, void* context);

    char const * get_error_message();
    void set_error_message(std::string const& error);
    int id() const;

private:
    mutable std::recursive_mutex mutex; // Protects all members of *this

    /* todo: race condition. protobuf does not guarantee that callbacks will be synchronized. potential
             race in session, last_buffer_id */
    mir::protobuf::DisplayServer::Stub & server;
    mir::protobuf::TrustedSession session;
    mir::protobuf::TrustedSessionParameters parameters;
    mir::protobuf::Void protobuf_void;
    std::string error_message;

    std::shared_ptr<mir::client::TrustedSessionControl> const trusted_session_control;
    int trusted_session_control_fn_id;

    MirWaitHandle start_wait_handle;
    MirWaitHandle stop_wait_handle;

    void done_start(mir_trusted_session_callback callback, void* context);
    void done_stop(mir_trusted_session_callback callback, void* context);
};

#endif /* MIR_CLIENT_MIR_TRUSTED_SESSION_H_ */

