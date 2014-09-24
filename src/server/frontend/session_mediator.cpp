/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "session_mediator.h"
#include "client_buffer_tracker.h"

#include "mir/frontend/session_mediator_report.h"
#include "mir/frontend/shell.h"
#include "mir/frontend/session.h"
#include "mir/frontend/surface.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/frontend/display_changer.h"
#include "resource_cache.h"
#include "mir_toolkit/common.h"
#include "mir/graphics/buffer_id.h"
#include "mir/graphics/buffer.h"
#include "mir/input/cursor_images.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/geometry/dimensions.h"
#include "mir/frontend/display_changer.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/pixel_format_utils.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/drm_authenticator.h"
#include "mir/frontend/client_constants.h"
#include "mir/frontend/event_sink.h"
#include "mir/frontend/screencast.h"
#include "mir/frontend/prompt_session.h"
#include "mir/scene/prompt_session_creation_parameters.h"
#include "mir/fd.h"

#include "mir/geometry/rectangles.h"
#include "surface_tracker.h"
#include "client_buffer_tracker.h"
#include "protobuf_buffer_packer.h"

#include <boost/exception/get_error_info.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <mutex>
#include <functional>

namespace ms = mir::scene;
namespace mf = mir::frontend;
namespace mfd=mir::frontend::detail;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;

mf::SessionMediator::SessionMediator(
    std::shared_ptr<frontend::Shell> const& shell,
    std::shared_ptr<graphics::Platform> const & graphics_platform,
    std::shared_ptr<mf::DisplayChanger> const& display_changer,
    std::vector<MirPixelFormat> const& surface_pixel_formats,
    std::shared_ptr<SessionMediatorReport> const& report,
    std::shared_ptr<EventSink> const& sender,
    std::shared_ptr<MessageResourceCache> const& resource_cache,
    std::shared_ptr<Screencast> const& screencast,
    ConnectionContext const& connection_context,
    std::shared_ptr<mi::CursorImages> const& cursor_images) :
    client_pid_(0),
    shell(shell),
    graphics_platform(graphics_platform),
    ipc_operations(graphics_platform->make_ipc_operations()),
    surface_pixel_formats(surface_pixel_formats),
    display_changer(display_changer),
    report(report),
    event_sink(sender),
    resource_cache(resource_cache),
    screencast(screencast),
    connection_context(connection_context),
    cursor_images(cursor_images),
    surface_tracker{static_cast<size_t>(client_buffer_cache_size)}
{
}

mf::SessionMediator::~SessionMediator() noexcept
{
    if (auto session = weak_session.lock())
    {
        report->session_error(session->name(), __PRETTY_FUNCTION__, "connection dropped without disconnect");
        shell->close_session(session);
    }
}

void mf::SessionMediator::client_pid(int pid)
{
    client_pid_ = pid;
}

void mf::SessionMediator::connect(
    ::google::protobuf::RpcController*,
    const ::mir::protobuf::ConnectParameters* request,
    ::mir::protobuf::Connection* response,
    ::google::protobuf::Closure* done)
{
    report->session_connect_called(request->application_name());

    auto const session = shell->open_session(client_pid_, request->application_name(), event_sink);
    {
        std::lock_guard<std::mutex> lock(session_mutex);
        weak_session = session;
    }
    connection_context.handle_client_connect(session);

    auto ipc_package = ipc_operations->connection_ipc_package();
    auto platform = response->mutable_platform();

    for (auto& data : ipc_package->ipc_data)
        platform->add_data(data);

    for (auto& ipc_fds : ipc_package->ipc_fds)
        platform->add_fd(ipc_fds);

    auto display_config = display_changer->active_configuration();
    auto protobuf_config = response->mutable_display_configuration();
    mfd::pack_protobuf_display_configuration(*protobuf_config, *display_config);

    for (auto pf : surface_pixel_formats)
        response->add_surface_pixel_format(static_cast<::google::protobuf::uint32>(pf));

    resource_cache->save_resource(response, ipc_package);

    done->Run();
}

void mf::SessionMediator::advance_buffer(
    SurfaceId surf_id,
    Surface& surface,
    std::function<void(graphics::Buffer*, graphics::BufferIpcMsgType)> complete)
{
    auto client_buffer = surface_tracker.last_buffer(surf_id);
    if (client_buffer)
    {
        //TODO: once we are doing an exchange_buffer, we should use the request buffer
        static mir::protobuf::Buffer dummy_raw_msg;
        mfd::ProtobufBufferPacker dummy_msg{&dummy_raw_msg};
        ipc_operations->unpack_buffer(dummy_msg, *client_buffer);
    }

    surface.swap_buffers(
        client_buffer, 
        [this, surf_id, complete](mg::Buffer* new_buffer)
        {
            if (surface_tracker.track_buffer(surf_id, new_buffer))
                complete(new_buffer, mg::BufferIpcMsgType::update_msg);
            else
                complete(new_buffer, mg::BufferIpcMsgType::full_msg);
        });
}

void mf::SessionMediator::create_surface(
    google::protobuf::RpcController* /*controller*/,
    const mir::protobuf::SurfaceParameters* request,
    mir::protobuf::Surface* response,
    google::protobuf::Closure* done)
{

    auto const lock = std::make_shared<std::unique_lock<std::mutex>>(session_mutex);

    auto const session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_create_surface_called(session->name());

    auto const surf_id = session->create_surface(ms::SurfaceCreationParameters()
        .of_name(request->surface_name())
        .of_size(request->width(), request->height())
        .of_buffer_usage(static_cast<graphics::BufferUsage>(request->buffer_usage()))
        .of_pixel_format(static_cast<MirPixelFormat>(request->pixel_format()))
        .with_output_id(graphics::DisplayConfigurationOutputId(request->output_id())));

    auto surface = session->get_surface(surf_id);
    auto const& client_size = surface->client_size();
    response->mutable_id()->set_value(surf_id.as_value());
    response->set_width(client_size.width.as_uint32_t());
    response->set_height(client_size.height.as_uint32_t());
    response->set_pixel_format((int)surface->pixel_format());
    response->set_buffer_usage(request->buffer_usage());

    if (surface->supports_input())
        response->add_fd(surface->client_input_fd());
    
    for (unsigned int i = 0; i < mir_surface_attribs; i++)
    {
        auto setting = response->add_attributes();
        
        setting->mutable_surfaceid()->set_value(surf_id.as_value());
        setting->set_attrib(i);
        setting->set_ivalue(surface->query(static_cast<MirSurfaceAttrib>(i)));
    }

    advance_buffer(surf_id, *surface,
        [lock, this, response, done, session]
        (graphics::Buffer* client_buffer, graphics::BufferIpcMsgType msg_type)
        {
            lock->unlock();

            auto buffer = response->mutable_buffer();
            pack_protobuf_buffer(*buffer, client_buffer, msg_type);

            // TODO: NOTE: We use the ordering here to ensure the shell acts on the surface after the surface ID is sent over the wire.
            // This guarantees that notifications such as, gained focus, etc, can be correctly interpreted by the client.
            // To achieve this order we rely on done->Run() sending messages synchronously. As documented in mfd::SocketMessenger::send.
            // this will require additional synchronization if mfd::SocketMessenger::send changes.
            done->Run();
            shell->handle_surface_created(session);
        });
}

void mf::SessionMediator::next_buffer(
    ::google::protobuf::RpcController* /*controller*/,
    ::mir::protobuf::SurfaceId const* request,
    ::mir::protobuf::Buffer* response,
    ::google::protobuf::Closure* done)
{
    SurfaceId const surf_id{request->value()};

    auto const lock = std::make_shared<std::unique_lock<std::mutex>>(session_mutex);

    auto const session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_next_buffer_called(session->name());

    auto surface = session->get_surface(surf_id);

    advance_buffer(surf_id, *surface,
        [lock, this, response, done, session]
        (graphics::Buffer* client_buffer, graphics::BufferIpcMsgType msg_type)
        {
            lock->unlock();

            pack_protobuf_buffer(*response, client_buffer, msg_type);

            done->Run();
        });
}

void mf::SessionMediator::exchange_buffer(
    google::protobuf::RpcController*,
    mir::protobuf::BufferRequest const* request,
    mir::protobuf::Buffer* response,
    google::protobuf::Closure* done)
{
    mf::SurfaceId const surface_id{request->id().value()};
    mg::BufferID const buffer_id{static_cast<uint32_t>(request->buffer().buffer_id())};

    auto const lock = std::make_shared<std::unique_lock<std::mutex>>(session_mutex);
    auto const session = weak_session.lock();
    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_exchange_buffer_called(session->name());

    auto const& surface = session->get_surface(surface_id);
    surface->swap_buffers(
        surface_tracker.buffer_from(buffer_id),
        [this, surface_id, lock, response, done](mg::Buffer* new_buffer)
        {
            lock->unlock();

            if (surface_tracker.track_buffer(surface_id, new_buffer))
                pack_protobuf_buffer(*response, new_buffer, mg::BufferIpcMsgType::update_msg);
            else
                pack_protobuf_buffer(*response, new_buffer, mg::BufferIpcMsgType::full_msg);

            done->Run();
        });
}

void mf::SessionMediator::release_surface(
    google::protobuf::RpcController* /*controller*/,
    const mir::protobuf::SurfaceId* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    {
        std::unique_lock<std::mutex> lock(session_mutex);

        auto session = weak_session.lock();

        if (session.get() == nullptr)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

        report->session_release_surface_called(session->name());

        auto const id = SurfaceId(request->value());

        session->destroy_surface(id);
        surface_tracker.remove_surface(id);
    }

    // TODO: We rely on this sending responses synchronously.
    done->Run();
}

void mf::SessionMediator::disconnect(
    google::protobuf::RpcController* /*controller*/,
    const mir::protobuf::Void* /*request*/,
    mir::protobuf::Void* /*response*/,
    google::protobuf::Closure* done)
{
    {
        std::unique_lock<std::mutex> lock(session_mutex);

        auto session = weak_session.lock();

        if (session.get() == nullptr)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

        report->session_disconnect_called(session->name());

        shell->close_session(session);
        weak_session.reset();
    }

    done->Run();
}

void mf::SessionMediator::configure_surface(
    google::protobuf::RpcController*, // controller,
    const mir::protobuf::SurfaceSetting* request,
    mir::protobuf::SurfaceSetting* response,
    google::protobuf::Closure* done)
{
    MirSurfaceAttrib attrib = static_cast<MirSurfaceAttrib>(request->attrib());

    // Required response fields:
    response->mutable_surfaceid()->CopyFrom(request->surfaceid());
    response->set_attrib(attrib);

    {
        std::unique_lock<std::mutex> lock(session_mutex);

        auto session = weak_session.lock();

        if (session.get() == nullptr)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

        report->session_configure_surface_called(session->name());

        auto const id = frontend::SurfaceId(request->surfaceid().value());
        int value = request->ivalue();
        auto const surface = session->get_surface(id);
        int newvalue = surface->configure(attrib, value);

        response->set_ivalue(newvalue);
    }

    done->Run();
}

void mf::SessionMediator::configure_display(
    ::google::protobuf::RpcController*,
    const ::mir::protobuf::DisplayConfiguration* request,
    ::mir::protobuf::DisplayConfiguration* response,
    ::google::protobuf::Closure* done)
{
    {
        std::unique_lock<std::mutex> lock(session_mutex);
        auto session = weak_session.lock();

        if (session.get() == nullptr)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

        report->session_configure_display_called(session->name());

        auto config = display_changer->active_configuration();

        config->for_each_output([&](mg::UserDisplayConfigurationOutput& dest){
            unsigned id = dest.id.as_value();
            int n = 0;
            for (; n < request->display_output_size(); ++n)
            {
                if (request->display_output(n).output_id() == id)
                    break;
            }
            if (n >= request->display_output_size())
                return;

            auto& src = request->display_output(n);
            dest.used = src.used();
            dest.top_left = geom::Point{src.position_x(),
                                        src.position_y()};
            dest.current_mode_index = src.current_mode();
            dest.current_format =
                static_cast<MirPixelFormat>(src.current_format());
            dest.power_mode = static_cast<MirPowerMode>(src.power_mode());
            dest.orientation = static_cast<MirOrientation>(src.orientation());
        });

        display_changer->configure(session, config);
        auto display_config = display_changer->active_configuration();
        mfd::pack_protobuf_display_configuration(*response, *display_config);
    }
    done->Run();
}

void mf::SessionMediator::create_screencast(
    google::protobuf::RpcController*,
    const mir::protobuf::ScreencastParameters* parameters,
    mir::protobuf::Screencast* protobuf_screencast,
    google::protobuf::Closure* done)
{
    static auto const msg_type = mg::BufferIpcMsgType::full_msg;

    geom::Rectangle const region{
        {parameters->region().left(), parameters->region().top()},
        {parameters->region().width(), parameters->region().height()}
    };
    geom::Size const size{parameters->width(), parameters->height()};
    MirPixelFormat const pixel_format = static_cast<MirPixelFormat>(parameters->pixel_format());

    auto screencast_session_id = screencast->create_session(region, size, pixel_format);
    auto buffer = screencast->capture(screencast_session_id);

    protobuf_screencast->mutable_screencast_id()->set_value(
        screencast_session_id.as_value());
    pack_protobuf_buffer(*protobuf_screencast->mutable_buffer(),
                         buffer.get(),
                         msg_type);

    done->Run();
}

void mf::SessionMediator::release_screencast(
    google::protobuf::RpcController*,
    const mir::protobuf::ScreencastId* protobuf_screencast_id,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    ScreencastSessionId const screencast_session_id{
        protobuf_screencast_id->value()};
    screencast->destroy_session(screencast_session_id);
    done->Run();
}

void mf::SessionMediator::screencast_buffer(
    google::protobuf::RpcController*,
    const mir::protobuf::ScreencastId* protobuf_screencast_id,
    mir::protobuf::Buffer* protobuf_buffer,
    google::protobuf::Closure* done)
{
    static auto const msg_type = mg::BufferIpcMsgType::update_msg;
    ScreencastSessionId const screencast_session_id{
        protobuf_screencast_id->value()};

    auto buffer = screencast->capture(screencast_session_id);

    pack_protobuf_buffer(*protobuf_buffer,
                         buffer.get(),
                         msg_type);

    done->Run();
}

std::function<void(std::shared_ptr<mf::Session> const&)> mf::SessionMediator::prompt_session_connect_handler() const
{
    return [this](std::shared_ptr<frontend::Session> const& session)
    {
        auto prompt_session = weak_prompt_session.lock();
        if (prompt_session.get() == nullptr)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid prompt session"));

        shell->add_prompt_provider_for(prompt_session, session);
    };
}

void mf::SessionMediator::configure_cursor(
    google::protobuf::RpcController*,
    mir::protobuf::CursorSetting const* cursor_request,
    mir::protobuf::Void* /* void_response */,
    google::protobuf::Closure* done)
{
    {
        std::unique_lock<std::mutex> lock(session_mutex);

        auto session = weak_session.lock();

        if (session.get() == nullptr)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

        report->session_configure_surface_cursor_called(session->name());

        auto const id = frontend::SurfaceId(cursor_request->surfaceid().value());
        auto const surface = session->get_surface(id);

        if (cursor_request->has_name())
        {
            auto const& image = cursor_images->image(cursor_request->name(), mi::default_cursor_size);
            surface->set_cursor_image(image);
        }
        else
        {
            surface->set_cursor_image({});
        }
    }
    done->Run();
}

void mf::SessionMediator::new_fds_for_prompt_providers(
    ::google::protobuf::RpcController* ,
    ::mir::protobuf::SocketFDRequest const* parameters,
    ::mir::protobuf::SocketFD* response,
    ::google::protobuf::Closure* done)
{
    {
        std::unique_lock<std::mutex> lock(session_mutex);
        auto session = weak_session.lock();

        if (session.get() == nullptr)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

        auto const connect_handler = prompt_session_connect_handler();

        auto const fds_requested = parameters->number();

        // < 1 is illogical, > 42 is unreasonable
        if (fds_requested < 1 || fds_requested > 42)
            BOOST_THROW_EXCEPTION(std::runtime_error("number of fds requested out of range"));

        for (auto i  = 0; i != fds_requested; ++i)
        {
            auto const fd = connection_context.fd_for_new_client(connect_handler);
            response->add_fd(fd);
            resource_cache->save_fd(response, mir::Fd{fd});
        }
    }

    done->Run();
}

void mf::SessionMediator::drm_auth_magic(
    google::protobuf::RpcController* /*controller*/,
    const mir::protobuf::DRMMagic* request,
    mir::protobuf::DRMAuthMagicStatus* response,
    google::protobuf::Closure* done)
{
    {
        std::unique_lock<std::mutex> lock(session_mutex);
        auto session = weak_session.lock();

        if (session.get() == nullptr)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

        report->session_drm_auth_magic_called(session->name());
    }

    auto const magic = static_cast<unsigned int>(request->magic());
    //FIXME: don't dynamic cast like this drm_auth_magic should be a part of PlatformIpcOperations 
    auto authenticator = std::dynamic_pointer_cast<mg::DRMAuthenticator>(graphics_platform);
    if (!authenticator)
        BOOST_THROW_EXCEPTION(std::logic_error("drm_auth_magic request not supported by the active platform"));

    try
    {
        authenticator->drm_auth_magic(magic);
        response->set_status_code(0);
    }
    catch (std::exception const& e)
    {
        auto errno_ptr = boost::get_error_info<boost::errinfo_errno>(e);

        if (errno_ptr != nullptr)
            response->set_status_code(*errno_ptr);
        else
            throw;
    }

    done->Run();
}

void mf::SessionMediator::start_prompt_session(
    ::google::protobuf::RpcController*,
    const ::mir::protobuf::PromptSessionParameters* request,
    ::mir::protobuf::Void* /*response*/,
    ::google::protobuf::Closure* done)
{
    {
        std::unique_lock<std::mutex> lock(session_mutex);
        auto const session = weak_session.lock();

        if (!session)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

        if (weak_prompt_session.lock())
            BOOST_THROW_EXCEPTION(std::runtime_error("Cannot start another prompt session"));

        ms::PromptSessionCreationParameters parameters;
        parameters.application_pid = request->application_pid();

        report->session_start_prompt_session_called(session->name(), parameters.application_pid);

        weak_prompt_session = shell->start_prompt_session_for(session, parameters);
    }
    done->Run();
}

void mf::SessionMediator::stop_prompt_session(
    ::google::protobuf::RpcController*,
    const ::mir::protobuf::Void*,
    ::mir::protobuf::Void*,
    ::google::protobuf::Closure* done)
{
    {
        std::unique_lock<std::mutex> lock(session_mutex);
        auto const session = weak_session.lock();

        if (!session)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

        auto const prompt_session = weak_prompt_session.lock();

        if (!prompt_session)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid prompt session"));

        weak_prompt_session.reset();

        report->session_stop_prompt_session_called(session->name());

        shell->stop_prompt_session(prompt_session);
    }
    done->Run();
}

void mf::SessionMediator::pack_protobuf_buffer(
    protobuf::Buffer& protobuf_buffer,
    graphics::Buffer* graphics_buffer,
    mg::BufferIpcMsgType buffer_msg_type)
{
    protobuf_buffer.set_buffer_id(graphics_buffer->id().as_value());

    mfd::ProtobufBufferPacker packer{&protobuf_buffer};
    ipc_operations->pack_buffer(packer, *graphics_buffer, buffer_msg_type);

    for(auto const& fd : packer.fds())
        resource_cache->save_fd(&protobuf_buffer, fd);
}
