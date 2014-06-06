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

#ifndef MIR_CLIENT_MIR_TRUST_SESSION_H_
#define MIR_CLIENT_MIR_TRUST_SESSION_H_

#include "mir_toolkit/mir_client_library.h"

#include "mir_protobuf.pb.h"
#include "mir_wait_handle.h"

#include <mutex>
#include <memory>
#include <atomic>

namespace mir
{
/// The client-side library implementation namespace
namespace client
{
class EventHandlerRegister;
}
}

struct MirTrustSession
{
public:
    MirTrustSession(mir::protobuf::DisplayServer& server,
                    std::shared_ptr<mir::client::EventHandlerRegister> const& event_handler_register);

    ~MirTrustSession();

    MirWaitHandle* start(pid_t pid, mir_trust_session_callback callback, void* context);
    MirWaitHandle* stop(mir_trust_session_callback callback, void* context);
    MirWaitHandle* add_trusted_session(pid_t pid, mir_trust_session_add_trusted_session_callback callback, void* context);

    void register_trust_session_event_callback(mir_trust_session_event_callback callback, void* context);

    char const* get_error_message();

    MirTrustSessionState get_state() const;

private:
    std::mutex mutable mutex; // Protects parameters, wait_handles & results
    mir::protobuf::DisplayServer& server;
    mir::protobuf::TrustedSession trusted_session;
    mir::protobuf::TrustSessionParameters parameters;
    mir::protobuf::Void add_result;
    mir::protobuf::Void protobuf_void;
    std::shared_ptr<mir::client::EventHandlerRegister> const event_handler_register;
    int event_handler_register_id;

    bool stop_sent{false}; // When handling a stopped event we need to know if we caused it
    MirWaitHandle start_wait_handle;
    MirWaitHandle stop_wait_handle;
    MirWaitHandle add_result_wait_handle;
    std::atomic<MirTrustSessionState> state;

    std::mutex mutable session_mutex; // Protects session
    mir::protobuf::Void session;

    std::mutex mutable event_handler_mutex; // Need another mutex for callback access to members
    std::function<void(MirTrustSessionState)> handle_trust_session_event;

    void set_state(MirTrustSessionState new_state);
    void done_start(mir_trust_session_callback callback, void* context);
    void done_stop(mir_trust_session_callback callback, void* context);
    void done_add_trusted_session(mir_trust_session_add_trusted_session_callback callback, void* context);
    MirTrustSession(MirTrustSession const&) = delete;
    MirTrustSession& operator=(MirTrustSession const&) = delete;
};

#endif /* MIR_CLIENT_MIR_TRUST_SESSION_H_ */

