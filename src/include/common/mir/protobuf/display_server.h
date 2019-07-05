/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#ifndef MIR_PROTOBUF_DISPLAY_SERVER_H_
#define MIR_PROTOBUF_DISPLAY_SERVER_H_

#include <google/protobuf/stubs/callback.h>
#include "mir_protobuf.pb.h"

namespace mir
{
namespace protobuf
{
class DisplayServer
{

public:
    virtual ~DisplayServer() = default;

    virtual void connect(
        mir::protobuf::ConnectParameters const* request,
        mir::protobuf::Connection* response,
        google::protobuf::Closure* done) = 0;
    virtual void disconnect(
        mir::protobuf::Void const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void create_surface(
        mir::protobuf::SurfaceParameters const* request,
        mir::protobuf::Surface* response,
        google::protobuf::Closure* done) = 0;
    virtual void modify_surface(
        mir::protobuf::SurfaceModifications const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void release_surface(
        mir::protobuf::SurfaceId const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void platform_operation(
        mir::protobuf::PlatformOperationMessage const* request,
        mir::protobuf::PlatformOperationMessage* response,
        google::protobuf::Closure* done) = 0;
    virtual void configure_surface(
        mir::protobuf::SurfaceSetting const* request,
        mir::protobuf::SurfaceSetting* response,
        google::protobuf::Closure* done) = 0;
    virtual void configure_display(
        mir::protobuf::DisplayConfiguration const* request,
        mir::protobuf::DisplayConfiguration* response,
        google::protobuf::Closure* done) = 0;
    virtual void remove_session_configuration(
        mir::protobuf::Void const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void set_base_display_configuration(
        mir::protobuf::DisplayConfiguration const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void preview_base_display_configuration(
        mir::protobuf::PreviewConfiguration const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void confirm_base_display_configuration(
        mir::protobuf::DisplayConfiguration const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void cancel_base_display_configuration_preview(
        mir::protobuf::Void const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void create_screencast(
        mir::protobuf::ScreencastParameters const* request,
        mir::protobuf::Screencast* response,
        google::protobuf::Closure* done) = 0;
    virtual void screencast_buffer(
        mir::protobuf::ScreencastId const* request,
        mir::protobuf::Buffer* response,
        google::protobuf::Closure* done) = 0;
    virtual void screencast_to_buffer(
        mir::protobuf::ScreencastRequest const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void release_screencast(
        mir::protobuf::ScreencastId const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void create_buffer_stream(
        mir::protobuf::BufferStreamParameters const* request,
        mir::protobuf::BufferStream* response,
        google::protobuf::Closure* done) = 0;
    virtual void release_buffer_stream(
        mir::protobuf::BufferStreamId const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void configure_cursor(
        mir::protobuf::CursorSetting const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void new_fds_for_prompt_providers(
        mir::protobuf::SocketFDRequest const* request,
        mir::protobuf::SocketFD* response,
        google::protobuf::Closure* done) = 0;
    virtual void start_prompt_session(
        mir::protobuf::PromptSessionParameters const* request,
        ::mir::protobuf::PromptSession* response,
        google::protobuf::Closure* done) = 0;
    virtual void stop_prompt_session(
        mir::protobuf::PromptSession const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void submit_buffer(
        mir::protobuf::BufferRequest const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void allocate_buffers(
        mir::protobuf::BufferAllocation const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void release_buffers(
        mir::protobuf::BufferRelease const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void request_persistent_surface_id(
        mir::protobuf::SurfaceId const* request,
        mir::protobuf::PersistentSurfaceId* response,
        google::protobuf::Closure* done) = 0;
    virtual void pong(
        mir::protobuf::PingEvent const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void configure_buffer_stream(
        mir::protobuf::StreamConfiguration const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void request_operation(
        mir::protobuf::RequestWithAuthority const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void apply_input_configuration(
        mir::protobuf::InputConfigurationRequest const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;
    virtual void set_base_input_configuration(
        mir::protobuf::InputConfigurationRequest const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) = 0;

protected:
    DisplayServer() = default;

private:
    DisplayServer(DisplayServer const&) = delete;
    void operator=(DisplayServer const&) = delete;
};
}
}

#endif //MIR_PROTOBUF_DISPLAY_SERVER_H_

