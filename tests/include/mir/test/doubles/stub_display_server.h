/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_DISPLAY_SERVER_H_
#define MIR_TEST_DOUBLES_STUB_DISPLAY_SERVER_H_

#include "src/server/frontend/display_server.h"

namespace mir
{
namespace test
{
namespace doubles
{
struct StubDisplayServer : public mir::frontend::detail::DisplayServer
{
    void client_pid(int /*pid*/) {}
    void connect(
        mir::protobuf::ConnectParameters const* /*request*/,
        mir::protobuf::Connection* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void disconnect(
        mir::protobuf::Void const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void create_surface(
        mir::protobuf::SurfaceParameters const* /*request*/,
        mir::protobuf::Surface* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void modify_surface(
        mir::protobuf::SurfaceModifications const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void next_buffer(
        mir::protobuf::SurfaceId const* /*request*/,
        mir::protobuf::Buffer* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void release_surface(
        mir::protobuf::SurfaceId const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void drm_auth_magic(
        mir::protobuf::DRMMagic const* /*request*/,
        mir::protobuf::DRMAuthMagicStatus* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void platform_operation(
        mir::protobuf::PlatformOperationMessage const* /*request*/,
        mir::protobuf::PlatformOperationMessage* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void configure_surface(
        mir::protobuf::SurfaceSetting const* /*request*/,
        mir::protobuf::SurfaceSetting* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void configure_display(
        mir::protobuf::DisplayConfiguration const* /*request*/,
        mir::protobuf::DisplayConfiguration* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void create_screencast(
        mir::protobuf::ScreencastParameters const* /*request*/,
        mir::protobuf::Screencast* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void screencast_buffer(
        mir::protobuf::ScreencastId const* /*request*/,
        mir::protobuf::Buffer* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void release_screencast(
        mir::protobuf::ScreencastId const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void create_buffer_stream(
        mir::protobuf::BufferStreamParameters const* /*request*/,
        mir::protobuf::BufferStream* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void release_buffer_stream(
        mir::protobuf::BufferStreamId const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void configure_cursor(
        mir::protobuf::CursorSetting const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void new_fds_for_prompt_providers(
        mir::protobuf::SocketFDRequest const* /*request*/,
        mir::protobuf::SocketFD* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void start_prompt_session(
        mir::protobuf::PromptSessionParameters const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void stop_prompt_session(
        mir::protobuf::Void const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void exchange_buffer(
        mir::protobuf::BufferRequest const* /*request*/,
        mir::protobuf::Buffer* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void submit_buffer(
        mir::protobuf::BufferRequest const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void allocate_buffers(
        mir::protobuf::BufferAllocation const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void release_buffers(
        mir::protobuf::BufferRelease const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void request_persistent_surface_id(
        mir::protobuf::SurfaceId const* /*request*/,
        mir::protobuf::PersistentSurfaceId* /*response*/,
        google::protobuf::Closure* /*done*/) {}
    void pong(
        mir::protobuf::PingEvent const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* /*done*/) {}
};

}
}
}

#endif
