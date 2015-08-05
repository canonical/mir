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

#ifndef MIR_CLIENT_MIR_PROMPT_SESSION_H_
#define MIR_CLIENT_MIR_PROMPT_SESSION_H_

#include "mir_toolkit/mir_client_library.h"

#include "mir_wait_handle.h"

#include <mutex>
#include <memory>
#include <atomic>

namespace mir
{
namespace protobuf
{
class PromptSessionParameters;
class SocketFD;
class Void;
}
/// The client-side library implementation namespace
namespace client
{
class EventHandlerRegister;
namespace rpc
{
class DisplayServer;
}
}
}

struct MirPromptSession
{
public:
    MirPromptSession(mir::client::rpc::DisplayServer& server,
                     std::shared_ptr<mir::client::EventHandlerRegister> const& event_handler_register);

    ~MirPromptSession();

    MirWaitHandle* start(pid_t application_pid, mir_prompt_session_callback callback, void* context);
    MirWaitHandle* stop(mir_prompt_session_callback callback, void* context);

    MirWaitHandle* new_fds_for_prompt_providers(
        unsigned int no_of_fds,
        mir_client_fd_callback callback,
        void * context);

    void register_prompt_session_state_change_callback(mir_prompt_session_state_change_callback callback, void* context);

    char const* get_error_message();

private:
    std::mutex mutable mutex; // Protects parameters, wait_handles & results
    mir::client::rpc::DisplayServer& server;
    std::unique_ptr<mir::protobuf::PromptSessionParameters> parameters;
    std::unique_ptr<mir::protobuf::Void> add_result;
    std::unique_ptr<mir::protobuf::Void> protobuf_void;
    std::unique_ptr<mir::protobuf::SocketFD> socket_fd_response;
    std::shared_ptr<mir::client::EventHandlerRegister> const event_handler_register;
    int const event_handler_register_id;

    MirWaitHandle start_wait_handle;
    MirWaitHandle stop_wait_handle;
    MirWaitHandle fds_for_prompt_providers_wait_handle;
    std::atomic<MirPromptSessionState> state;

    std::mutex mutable session_mutex; // Protects session
    std::unique_ptr<mir::protobuf::Void> session;

    std::mutex mutable event_handler_mutex; // Need another mutex for callback access to members
    std::function<void(MirPromptSessionState)> handle_prompt_session_state_change;

    void set_state(MirPromptSessionState new_state);
    void done_start(mir_prompt_session_callback callback, void* context);
    void done_stop(mir_prompt_session_callback callback, void* context);
    void done_fds_for_prompt_providers(mir_client_fd_callback callback, void* context);
    MirPromptSession(MirPromptSession const&) = delete;
    MirPromptSession& operator=(MirPromptSession const&) = delete;
};

#endif /* MIR_CLIENT_MIR_PROMPT_SESSION_H_ */

