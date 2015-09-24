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
#include "reordering_message_sender.h"
#include "event_sink_factory.h"

#include "mir/frontend/session_mediator_report.h"
#include "mir/frontend/shell.h"
#include "mir/frontend/session.h"
#include "mir/frontend/surface.h"
#include "mir/shell/surface_specification.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/coordinate_translator.h"
#include "mir/scene/application_not_responding_detector.h"
#include "mir/frontend/display_changer.h"
#include "resource_cache.h"
#include "mir_toolkit/common.h"
#include "mir/graphics/buffer_id.h"
#include "mir/graphics/buffer.h"
#include "mir/input/cursor_images.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/geometry/dimensions.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/pixel_format_utils.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/platform_operation_message.h"
#include "mir/frontend/client_constants.h"
#include "mir/frontend/event_sink.h"
#include "mir/frontend/screencast.h"
#include "mir/frontend/prompt_session.h"
#include "mir/frontend/buffer_stream.h"
#include "mir/scene/prompt_session_creation_parameters.h"
#include "mir/fd.h"

#include "mir/geometry/rectangles.h"
#include "buffer_stream_tracker.h"
#include "client_buffer_tracker.h"
#include "protobuf_buffer_packer.h"

#include "mir_toolkit/client_types.h"

#include <boost/exception/get_error_info.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <mutex>
#include <thread>
#include <functional>
#include <cstring>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mf = mir::frontend;
namespace mfd=mir::frontend::detail;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;

mf::SessionMediator::SessionMediator(
    std::shared_ptr<mf::Shell> const& shell,
    std::shared_ptr<mg::PlatformIpcOperations> const& ipc_operations,
    std::shared_ptr<mf::DisplayChanger> const& display_changer,
    std::vector<MirPixelFormat> const& surface_pixel_formats,
    std::shared_ptr<SessionMediatorReport> const& report,
    std::shared_ptr<mf::EventSinkFactory> const& sink_factory,
    std::shared_ptr<mf::MessageSender> const& message_sender,
    std::shared_ptr<MessageResourceCache> const& resource_cache,
    std::shared_ptr<Screencast> const& screencast,
    ConnectionContext const& connection_context,
    std::shared_ptr<mi::CursorImages> const& cursor_images,
    std::shared_ptr<scene::CoordinateTranslator> const& translator,
    std::shared_ptr<scene::ApplicationNotRespondingDetector> const& anr_detector) :
    client_pid_(0),
    shell(shell),
    ipc_operations(ipc_operations),
    surface_pixel_formats(surface_pixel_formats),
    display_changer(display_changer),
    report(report),
    sink_factory{sink_factory},
    event_sink{sink_factory->create_sink(message_sender)},
    message_sender{message_sender},
    resource_cache(resource_cache),
    screencast(screencast),
    connection_context(connection_context),
    cursor_images(cursor_images),
    translator{translator},
    anr_detector{anr_detector},
    buffer_stream_tracker{static_cast<size_t>(client_buffer_cache_size)}
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
    const ::mir::protobuf::ConnectParameters* request,
    ::mir::protobuf::Connection* response,
    ::google::protobuf::Closure* done)
{
    report->session_connect_called(request->application_name());

    auto const session = shell->open_session(client_pid_, request->application_name(), event_sink);
    weak_session = session;
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
    BufferStreamId stream_id,
    BufferStream& stream,
    graphics::Buffer* old_buffer,
    std::function<void(graphics::Buffer*, graphics::BufferIpcMsgType)> complete)
{
    stream.swap_buffers(
        old_buffer,
        [this, stream_id, complete](mg::Buffer* new_buffer)
        {
            if (!new_buffer || buffer_stream_tracker.track_buffer(stream_id, new_buffer))
                complete(new_buffer, mg::BufferIpcMsgType::update_msg);
            else
                complete(new_buffer, mg::BufferIpcMsgType::full_msg);
        });
}

namespace
{
template<typename T>
std::vector<geom::Rectangle>
extract_input_shape_from(T const& params)
{
    std::vector<geom::Rectangle> shapes;
    if (params->input_shape_size() > 0)
    {
        for (auto& rect : params->input_shape())
        {
            shapes.push_back(geom::Rectangle(
                geom::Point{rect.left(), rect.top()},
                geom::Size{rect.width(), rect.height()})
            );
        }
    }
    return shapes;
}
}

void mf::SessionMediator::create_surface(
    const mir::protobuf::SurfaceParameters* request,
    mir::protobuf::Surface* response,
    google::protobuf::Closure* done)
{
    auto const session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_create_surface_called(session->name());

    auto params = ms::SurfaceCreationParameters()
        .of_size(request->width(), request->height())
        .of_buffer_usage(static_cast<graphics::BufferUsage>(request->buffer_usage()))
        .of_pixel_format(static_cast<MirPixelFormat>(request->pixel_format()));

    if (request->has_surface_name())
        params.of_name(request->surface_name());

    if (request->has_output_id())
        params.with_output_id(graphics::DisplayConfigurationOutputId(request->output_id()));

    if (request->has_type())
        params.of_type(static_cast<MirSurfaceType>(request->type()));

    if (request->has_state())
        params.with_state(static_cast<MirSurfaceState>(request->state()));

    if (request->has_pref_orientation())
        params.with_preferred_orientation(static_cast<MirOrientationMode>(request->pref_orientation()));

    if (request->has_parent_id())
        params.with_parent_id(SurfaceId{request->parent_id()});


    if (request->has_parent_persistent_id())
    {
        auto persistent_id = request->parent_persistent_id().value();
        params.parent = shell->surface_for_id(persistent_id);
    }

    if (request->has_aux_rect())
    {
        params.with_aux_rect(geom::Rectangle{
            {request->aux_rect().left(), request->aux_rect().top()},
            {request->aux_rect().width(), request->aux_rect().height()}
        });
    }

    if (request->has_edge_attachment())
        params.with_edge_attachment(static_cast<MirEdgeAttachment>(request->edge_attachment()));

    #define COPY_IF_SET(field)\
        if (request->has_##field())\
            params.field = decltype(params.field.value())(request->field())

    COPY_IF_SET(min_width);
    COPY_IF_SET(min_height);
    COPY_IF_SET(max_width);
    COPY_IF_SET(max_height);
    COPY_IF_SET(width_inc);
    COPY_IF_SET(height_inc);

    #undef COPY_IF_SET

    if (request->has_min_aspect())
        params.min_aspect = { request->min_aspect().width(), request->min_aspect().height()};

    if (request->has_max_aspect())
        params.max_aspect = { request->max_aspect().width(), request->max_aspect().height()};

    params.input_shape = extract_input_shape_from(request);

    auto buffering_sender = std::make_shared<mf::ReorderingMessageSender>(message_sender);
    std::shared_ptr<mf::EventSink> sink = sink_factory->create_sink(buffering_sender);

    auto const surf_id = shell->create_surface(session, params, sink);
    auto stream_id = mf::BufferStreamId(surf_id.as_value());

    auto surface = session->get_surface(surf_id);
    auto stream = session->get_buffer_stream(stream_id);
    auto const& client_size = surface->client_size();
    response->mutable_id()->set_value(surf_id.as_value());
    response->set_width(client_size.width.as_uint32_t());
    response->set_height(client_size.height.as_uint32_t());

    // TODO: Deprecate
    response->set_pixel_format(stream->pixel_format());
    response->set_buffer_usage(request->buffer_usage());

    response->mutable_buffer_stream()->set_pixel_format(stream->pixel_format());
    response->mutable_buffer_stream()->set_buffer_usage(request->buffer_usage());

    if (surface->supports_input())
        response->add_fd(surface->client_input_fd());
    
    for (unsigned int i = 0; i < mir_surface_attribs; i++)
    {
        auto setting = response->add_attributes();
        
        setting->mutable_surfaceid()->set_value(surf_id.as_value());
        setting->set_attrib(i);
        setting->set_ivalue(shell->get_surface_attribute(session, surf_id, static_cast<MirSurfaceAttrib>(i)));
    }

    advance_buffer(stream_id, *stream, buffer_stream_tracker.last_buffer(stream_id),
        [this, buffering_sender, surf_id, response, done, session]
        (graphics::Buffer* client_buffer, graphics::BufferIpcMsgType msg_type)
        {
            response->mutable_buffer_stream()->mutable_id()->set_value(surf_id.as_value());
            if (client_buffer)
                pack_protobuf_buffer(*response->mutable_buffer_stream()->mutable_buffer(), client_buffer, msg_type);


            // Send the create_surface reply first...
            done->Run();

            // ...then uncork the message sender, sending all buffered surface events.
            buffering_sender->uncork();
        });
}

void mf::SessionMediator::next_buffer(
    ::mir::protobuf::SurfaceId const* request,
    ::mir::protobuf::Buffer* response,
    ::google::protobuf::Closure* done)
{
    SurfaceId const surf_id{request->value()};

    auto const session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_next_buffer_called(session->name());

    auto surface = session->get_surface(surf_id);
    auto stream = surface->primary_buffer_stream();
    auto stream_id = mf::BufferStreamId{surf_id.as_value()};

    advance_buffer(stream_id, *stream, buffer_stream_tracker.last_buffer(stream_id),
        [this, response, done]
        (graphics::Buffer* client_buffer, graphics::BufferIpcMsgType msg_type)
        {
            pack_protobuf_buffer(*response, client_buffer, msg_type);
            done->Run();
        });
}

void mf::SessionMediator::exchange_buffer(
    mir::protobuf::BufferRequest const* request,
    mir::protobuf::Buffer* response,
    google::protobuf::Closure* done)
{
    mf::BufferStreamId const stream_id{request->id().value()};

    mg::BufferID const buffer_id{static_cast<uint32_t>(request->buffer().buffer_id())};

    mfd::ProtobufBufferPacker request_msg{const_cast<mir::protobuf::Buffer*>(&request->buffer())};
    ipc_operations->unpack_buffer(request_msg, *buffer_stream_tracker.last_buffer(stream_id));

    auto const session = weak_session.lock();
    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_exchange_buffer_called(session->name());

    auto const& surface = session->get_buffer_stream(stream_id);
    advance_buffer(stream_id, *surface, buffer_stream_tracker.buffer_from(buffer_id),
        [this, response, done]
        (graphics::Buffer* new_buffer, graphics::BufferIpcMsgType msg_type)
        {
            pack_protobuf_buffer(*response, new_buffer, msg_type);
            done->Run();
        });
}

void mf::SessionMediator::submit_buffer(
    mir::protobuf::BufferRequest const* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    auto const session = weak_session.lock();
    if (!session) BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));
    report->session_submit_buffer_called(session->name());
    
    mf::BufferStreamId const stream_id{request->id().value()};
    mg::BufferID const buffer_id{static_cast<uint32_t>(request->buffer().buffer_id())};
    auto stream = session->get_buffer_stream(stream_id);

    mfd::ProtobufBufferPacker request_msg{const_cast<mir::protobuf::Buffer*>(&request->buffer())};
    if (auto* buffer = buffer_stream_tracker.last_buffer(stream_id))
    {
        ipc_operations->unpack_buffer(request_msg, *buffer);
        stream->swap_buffers(buffer,
            [this, stream_id](mg::Buffer* new_buffer)
            {
                if (buffer_stream_tracker.track_buffer(stream_id, new_buffer))
                    event_sink->send_buffer(stream_id, *new_buffer, mg::BufferIpcMsgType::update_msg);
                else
                    event_sink->send_buffer(stream_id, *new_buffer, mg::BufferIpcMsgType::full_msg);
            });
    }
    else
    {
        stream->with_buffer(buffer_id, [&, this](mg::Buffer& buffer)
        {
            ipc_operations->unpack_buffer(request_msg, buffer);
            stream->swap_buffers(&buffer, [](mg::Buffer*) {});
        });
    }

    done->Run();
}

void mf::SessionMediator::allocate_buffers( 
    mir::protobuf::BufferAllocation const* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();
    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_allocate_buffers_called(session->name());
    mf::BufferStreamId stream_id{request->id().value()};
    auto stream = session->get_buffer_stream(stream_id);
    for (auto i = 0; i < request->buffer_requests().size(); i++)
    {
        auto const& req = request->buffer_requests(i);
        mg::BufferProperties properties(
            geom::Size{req.width(), req.height()},
            static_cast<MirPixelFormat>(req.pixel_format()),
           static_cast<mg::BufferUsage>(req.buffer_usage()));
        stream->allocate_buffer(properties);
    }
    done->Run();
}
 
void mf::SessionMediator::release_buffers(
    mir::protobuf::BufferRelease const* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();
    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_release_buffers_called(session->name());
    mf::BufferStreamId stream_id{request->id().value()};
    auto stream = session->get_buffer_stream(stream_id);
    for (auto i = 0; i < request->buffers().size(); i++)
        stream->remove_buffer(mg::BufferID{static_cast<uint32_t>(request->buffers(i).buffer_id())});
   done->Run();
}
 
void mf::SessionMediator::release_surface(
    const mir::protobuf::SurfaceId* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_release_surface_called(session->name());

    auto const id = SurfaceId(request->value());

    shell->destroy_surface(session, id);
    buffer_stream_tracker.remove_buffer_stream(BufferStreamId(request->value()));

    // TODO: We rely on this sending responses synchronously.
    done->Run();
}

void mf::SessionMediator::disconnect(
    const mir::protobuf::Void* /*request*/,
    mir::protobuf::Void* /*response*/,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_disconnect_called(session->name());

    shell->close_session(session);
    weak_session.reset();

    done->Run();
}

void mf::SessionMediator::configure_surface(
    const mir::protobuf::SurfaceSetting* request,
    mir::protobuf::SurfaceSetting* response,
    google::protobuf::Closure* done)
{
    MirSurfaceAttrib attrib = static_cast<MirSurfaceAttrib>(request->attrib());

    // Required response fields:
    response->mutable_surfaceid()->CopyFrom(request->surfaceid());
    response->set_attrib(attrib);

    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_configure_surface_called(session->name());

    auto const id = mf::SurfaceId(request->surfaceid().value());
    int value = request->ivalue();
    int newvalue = shell->set_surface_attribute(session, id, attrib, value);

    response->set_ivalue(newvalue);

    done->Run();
}

void mf::SessionMediator::modify_surface(
    const mir::protobuf::SurfaceModifications* request,
    mir::protobuf::Void* /*response*/,
    google::protobuf::Closure* done)
{
    auto const& surface_specification = request->surface_specification();

    auto const session = weak_session.lock();
    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    msh::SurfaceSpecification mods;

#define COPY_IF_SET(name)\
    if (surface_specification.has_##name())\
    mods.name = decltype(mods.name.value())(surface_specification.name())

    COPY_IF_SET(width);
    COPY_IF_SET(height);
    COPY_IF_SET(pixel_format);
    COPY_IF_SET(buffer_usage);
    COPY_IF_SET(name);
    COPY_IF_SET(output_id);
    COPY_IF_SET(type);
    COPY_IF_SET(state);
    COPY_IF_SET(preferred_orientation);
    COPY_IF_SET(parent_id);
    // aux_rect is a special case (below)
    COPY_IF_SET(edge_attachment);
    COPY_IF_SET(min_width);
    COPY_IF_SET(min_height);
    COPY_IF_SET(max_width);
    COPY_IF_SET(max_height);
    COPY_IF_SET(width_inc);
    COPY_IF_SET(height_inc);
    // min_aspect is a special case (below)
    // max_aspect is a special case (below)

#undef COPY_IF_SET
    if (surface_specification.stream_size() > 0)
    {
        std::vector<msh::StreamSpecification> stream_spec;
        for (auto& stream : surface_specification.stream())
        {
            stream_spec.emplace_back(
                msh::StreamSpecification{
                    mf::BufferStreamId{stream.id().value()},
                    geom::Displacement{stream.displacement_x(), stream.displacement_y()}});
        }
        mods.streams = std::move(stream_spec);
    }

    if (surface_specification.has_aux_rect())
    {
        auto const& rect = surface_specification.aux_rect();
        mods.aux_rect = {{rect.left(), rect.top()}, {rect.width(), rect.height()}};
    }

    if (surface_specification.has_min_aspect())
        mods.min_aspect =
        {
            surface_specification.min_aspect().width(),
            surface_specification.min_aspect().height()
        };

    if (surface_specification.has_max_aspect())
        mods.max_aspect =
        {
            surface_specification.max_aspect().width(),
            surface_specification.max_aspect().height()
        };

    mods.input_shape = extract_input_shape_from(&surface_specification);

    auto const id = mf::SurfaceId(request->surface_id().value());

    shell->modify_surface(session, id, mods);

    done->Run();
}

void mf::SessionMediator::configure_display(
    const ::mir::protobuf::DisplayConfiguration* request,
    ::mir::protobuf::DisplayConfiguration* response,
    ::google::protobuf::Closure* done)
{
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

    done->Run();
}

void mf::SessionMediator::create_screencast(
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

    protobuf_screencast->mutable_buffer_stream()->mutable_id()->set_value(
        screencast_session_id.as_value());
    pack_protobuf_buffer(*protobuf_screencast->mutable_buffer_stream()->mutable_buffer(),
                         buffer.get(),
                         msg_type);

    done->Run();
}

void mf::SessionMediator::release_screencast(
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

void mf::SessionMediator::create_buffer_stream(
    mir::protobuf::BufferStreamParameters const* request,
    mir::protobuf::BufferStream* response,
    google::protobuf::Closure* done)
{
    auto const session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_create_surface_called(session->name());
    
    auto const usage = (request->buffer_usage() == mir_buffer_usage_hardware) ?
        mg::BufferUsage::hardware : mg::BufferUsage::software;

    auto stream_size = geom::Size{geom::Width{request->width()}, geom::Height{request->height()}};
    mg::BufferProperties props(stream_size,
        static_cast<MirPixelFormat>(request->pixel_format()),
        usage);
    
    auto const buffer_stream_id = session->create_buffer_stream(props);
    auto stream = session->get_buffer_stream(buffer_stream_id);
    
    response->mutable_id()->set_value(buffer_stream_id.as_value());
    response->set_pixel_format(stream->pixel_format());

    // TODO: Is it guaranteed we get the buffer usage we want?
    response->set_buffer_usage(request->buffer_usage());

    advance_buffer(buffer_stream_id, *stream, buffer_stream_tracker.last_buffer(buffer_stream_id),
        [this, response, done, session]
        (graphics::Buffer* client_buffer, graphics::BufferIpcMsgType msg_type)
        {
            auto buffer = response->mutable_buffer();
            if (client_buffer)
                pack_protobuf_buffer(*buffer, client_buffer, msg_type);

            done->Run();
        });
}

void mf::SessionMediator::release_buffer_stream(
    const mir::protobuf::BufferStreamId* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_create_surface_called(session->name());

    auto const id = BufferStreamId(request->value());

    session->destroy_buffer_stream(id);
    buffer_stream_tracker.remove_buffer_stream(id);

    done->Run();
}


std::function<void(std::shared_ptr<mf::Session> const&)> mf::SessionMediator::prompt_session_connect_handler() const
{
    return [this](std::shared_ptr<mf::Session> const& session)
    {
        auto prompt_session = weak_prompt_session.lock();
        if (prompt_session.get() == nullptr)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid prompt session"));

        shell->add_prompt_provider_for(prompt_session, session);
    };
}

namespace
{
void throw_if_unsuitable_for_cursor(mf::BufferStream& stream)
{
    if (stream.pixel_format() != mir_pixel_format_argb_8888)
        BOOST_THROW_EXCEPTION(std::logic_error("Only argb8888 buffer streams may currently be attached to the cursor"));
}
}

void mf::SessionMediator::configure_cursor(
    mir::protobuf::CursorSetting const* cursor_request,
    mir::protobuf::Void* /* void_response */,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_configure_surface_cursor_called(session->name());

    auto const id = mf::SurfaceId(cursor_request->surfaceid().value());
    auto const surface = session->get_surface(id);

    if (cursor_request->has_name())
    {
        auto const& image = cursor_images->image(cursor_request->name(), mi::default_cursor_size);
        surface->set_cursor_image(image);
    }
    else if (cursor_request->has_buffer_stream())
    {
        auto const& stream_id = mf::BufferStreamId(cursor_request->buffer_stream().value());
        auto hotspot = geom::Displacement{cursor_request->hotspot_x(), cursor_request->hotspot_y()};
        auto stream = session->get_buffer_stream(stream_id);

        throw_if_unsuitable_for_cursor(*stream);

        surface->set_cursor_stream(stream, hotspot);
    }
    else
    {
        surface->set_cursor_image({});
    }

    done->Run();
}

void mf::SessionMediator::new_fds_for_prompt_providers(
    ::mir::protobuf::SocketFDRequest const* parameters,
    ::mir::protobuf::SocketFD* response,
    ::google::protobuf::Closure* done)
{
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

    done->Run();
}

void mf::SessionMediator::pong(
    mir::protobuf::PingEvent const* /*request*/,
    mir::protobuf::Void* /* response */,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    anr_detector->pong_received(session.get());
    done->Run();
}

void mf::SessionMediator::translate_surface_to_screen(
    ::mir::protobuf::CoordinateTranslationRequest const* request,
    ::mir::protobuf::CoordinateTranslationResponse* response,
    ::google::protobuf::Closure *done)
{
    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    auto const id = mf::SurfaceId(request->surfaceid().value());

    auto const coords = translator->surface_to_screen(session->get_surface(id),
                                                      request->x(),
                                                      request->y());

    response->set_x(coords.x.as_uint32_t());
    response->set_y(coords.y.as_uint32_t());

    done->Run();
}

void mf::SessionMediator::platform_operation(
    mir::protobuf::PlatformOperationMessage const* request,
    mir::protobuf::PlatformOperationMessage* response,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    mg::PlatformOperationMessage platform_request;
    unsigned int const opcode = request->opcode();
    platform_request.data.assign(request->data().begin(),
                                 request->data().end());
    platform_request.fds.assign(request->fd().begin(),
                                request->fd().end());

    auto const& platform_response = ipc_operations->platform_operation(opcode, platform_request);

    response->set_opcode(opcode);
    response->set_data(platform_response.data.data(),
                       platform_response.data.size());
    for (auto fd : platform_response.fds)
    {
        response->add_fd(fd);
        resource_cache->save_fd(response, mir::Fd{fd});
    }

    done->Run();
}

void mf::SessionMediator::start_prompt_session(
    const ::mir::protobuf::PromptSessionParameters* request,
    ::mir::protobuf::Void* /*response*/,
    ::google::protobuf::Closure* done)
{
    auto const session = weak_session.lock();

    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    if (weak_prompt_session.lock())
        BOOST_THROW_EXCEPTION(std::runtime_error("Cannot start another prompt session"));

    ms::PromptSessionCreationParameters parameters;
    parameters.application_pid = request->application_pid();

    report->session_start_prompt_session_called(session->name(), parameters.application_pid);

    weak_prompt_session = shell->start_prompt_session_for(session, parameters);

    done->Run();
}

void mf::SessionMediator::stop_prompt_session(
    const ::mir::protobuf::Void*,
    ::mir::protobuf::Void*,
    ::google::protobuf::Closure* done)
{
    auto const session = weak_session.lock();

    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    auto const prompt_session = weak_prompt_session.lock();

    if (!prompt_session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid prompt session"));

    weak_prompt_session.reset();

    report->session_stop_prompt_session_called(session->name());

    shell->stop_prompt_session(prompt_session);

    done->Run();
}

void mf::SessionMediator::request_persistent_surface_id(
    mir::protobuf::SurfaceId const* request,
    mir::protobuf::PersistentSurfaceId* response,
    google::protobuf::Closure* done)
{
    auto const session = weak_session.lock();

    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    auto buffer = shell->persistent_id_for(session, mf::SurfaceId{request->value()});

    *response->mutable_value() = std::string{buffer.begin(), buffer.end()};

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

void mf::SessionMediator::configure_buffer_stream(
    mir::protobuf::StreamConfiguration const* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    auto const session = weak_session.lock();
    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    auto stream = session->get_buffer_stream(mf::BufferStreamId(request->id().value()));
    if (request->has_swapinterval())
        stream->allow_framedropping(request->swapinterval() == 0);
    if (request->has_scale())
        stream->set_scale(request->scale());

    done->Run();
}
