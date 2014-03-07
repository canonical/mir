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

#ifndef MIR_CLIENT_MIR_TRUST_SESSION_H_
#define MIR_CLIENT_MIR_TRUST_SESSION_H_

#include "mir_protobuf.pb.h"
#include "mir_wait_handle.h"
#include "client_trust_session.h"

#include <mutex>
#include <memory>
#include <atomic>

namespace mir
{
/// The client-side library implementation namespace
namespace client
{
class TrustSessionControl;
}
}

struct MirTrustSession : public mir::client::TrustSession
{
public:
    MirTrustSession(mir::protobuf::DisplayServer::Stub & server,
                      std::shared_ptr<mir::client::TrustSessionControl> const& trust_session_control);

    ~MirTrustSession();

    MirTrustSession(MirTrustSession const &) = delete;
    MirTrustSession& operator=(MirTrustSession const &) = delete;

    MirTrustSessionAddApplicationResult add_app_with_pid(pid_t pid);

    MirWaitHandle* start(mir_trust_session_callback callback, void * context);
    MirWaitHandle* stop(mir_trust_session_callback callback, void * context);

    void register_trust_session_event_callback(mir_trust_session_event_callback callback, void* context);

    char const * get_error_message();
    void set_error_message(std::string const& error);

    MirTrustSessionState get_state() const;

private:
    mutable std::recursive_mutex mutex; // Protects all members of *this

    /* todo: race condition. protobuf does not guarantee that callbacks will be synchronized. potential
             race in session, last_buffer_id */
    mir::protobuf::DisplayServer::Stub & server;
    mir::protobuf::TrustSession session;
    mir::protobuf::TrustSessionParameters parameters;
    mir::protobuf::Void protobuf_void;
    std::string error_message;

    std::shared_ptr<mir::client::TrustSessionControl> const trust_session_control;
    std::function<void(MirTrustSessionState)> handle_trust_session_event;
    int trust_session_control_fn_id;

    MirWaitHandle start_wait_handle;
    MirWaitHandle stop_wait_handle;
    std::atomic<MirTrustSessionState> state;

    void done_start(mir_trust_session_callback callback, void* context);
    void done_stop(mir_trust_session_callback callback, void* context);
};

#endif /* MIR_CLIENT_MIR_TRUST_SESSION_H_ */

