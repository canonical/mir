/*
 * Copyright © 2012-2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "mir/frontend/session_mediator_observer.h"
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
#include "mir/graphics/gamma_curves.h"
#include "mir/frontend/client_constants.h"
#include "mir/frontend/event_sink.h"
#include "mir/frontend/screencast.h"
#include "mir/frontend/prompt_session.h"
#include "mir/frontend/buffer_stream.h"
#include "mir/frontend/input_configuration_changer.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/mir_input_config.h"
#include "mir/input/mir_input_config_serialization.h"
#include "mir/input/mir_touchpad_config.h"
#include "mir/input/mir_pointer_config.h"
#include "mir/input/mir_keyboard_config.h"
#include "mir/input/device.h"
#include "mir/scene/prompt_session_creation_parameters.h"
#include "mir/fd.h"
#include "mir/cookie/authority.h"
#include "mir/module_properties.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/executor.h"

#include "mir/geometry/rectangles.h"
#include "protobuf_buffer_packer.h"
#include "protobuf_input_converter.h"

#include "mir_toolkit/client_types.h"
#include "mir_toolkit/cursors.h"

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

namespace
{
mg::GammaCurve convert_string_to_gamma_curve(std::string const& str_bytes)
{
    mg::GammaCurve out(str_bytes.size() / (sizeof(mg::GammaCurve::value_type) / sizeof(char)));
    std::copy(std::begin(str_bytes), std::end(str_bytes), reinterpret_cast<char*>(out.data()));
    return out;
}
}

mf::SessionMediator::SessionMediator(
    std::shared_ptr<mf::Shell> const& shell,
    std::shared_ptr<mg::PlatformIpcOperations> const& ipc_operations,
    std::shared_ptr<mf::DisplayChanger> const& display_changer,
    std::vector<MirPixelFormat> const& surface_pixel_formats,
    std::shared_ptr<SessionMediatorObserver> const& observer,
    std::shared_ptr<mf::EventSinkFactory> const& sink_factory,
    std::shared_ptr<mf::MessageSender> const& message_sender,
    std::shared_ptr<MessageResourceCache> const& resource_cache,
    std::shared_ptr<Screencast> const& screencast,
    ConnectionContext const& connection_context,
    std::shared_ptr<mi::CursorImages> const& cursor_images,
    std::shared_ptr<scene::CoordinateTranslator> const& translator,
    std::shared_ptr<scene::ApplicationNotRespondingDetector> const& anr_detector,
    std::shared_ptr<mir::cookie::Authority> const& cookie_authority,
    std::shared_ptr<mf::InputConfigurationChanger> const& input_changer,
    std::vector<mir::ExtensionDescription> const& extensions,
    std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
    mir::Executor& executor) :
    client_pid_(0),
    shell(shell),
    ipc_operations(ipc_operations),
    surface_pixel_formats(surface_pixel_formats),
    display_changer(display_changer),
    observer(observer),
    sink_factory{sink_factory},
    event_sink{sink_factory->create_sink(message_sender)},
    message_sender{message_sender},
    resource_cache(resource_cache),
    screencast(screencast),
    connection_context(connection_context),
    cursor_images(cursor_images),
    translator{translator},
    anr_detector{anr_detector},
    cookie_authority(cookie_authority),
    input_changer(input_changer),
    extensions(extensions),
    allocator{allocator},
    executor{executor}
{
}

mf::SessionMediator::~SessionMediator() noexcept
{
    if (auto session = weak_session.lock())
    {
        observer->session_error(session->name(), __PRETTY_FUNCTION__, "connection dropped without disconnect");
        shell->close_session(session);
    }
    destroy_screencast_sessions();
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
    observer->session_connect_called(request->application_name());

    auto const session = shell->open_session(client_pid_, request->application_name(), event_sink);
    weak_session = session;
    connection_context.handle_client_connect(session);

    auto ipc_package = ipc_operations->connection_ipc_package();
    auto platform = response->mutable_platform();

    for (auto& data : ipc_package->ipc_data)
        platform->add_data(data);

    for (auto& ipc_fds : ipc_package->ipc_fds)
        platform->add_fd(ipc_fds);

    if (auto const graphics_module = ipc_package->graphics_module)
    {
        auto const module = platform->mutable_graphics_module();

        module->set_name(graphics_module->name);
        module->set_major_version(graphics_module->major_version);
        module->set_minor_version(graphics_module->minor_version);
        module->set_micro_version(graphics_module->micro_version);
        module->set_file(graphics_module->file);
    }

    auto display_config = display_changer->base_configuration();
    auto protobuf_config = response->mutable_display_configuration();
    mfd::pack_protobuf_display_configuration(*protobuf_config, *display_config);

    response->set_input_configuration(mi::serialize_input_config(input_changer->base_configuration()));

    for (auto pf : surface_pixel_formats)
        response->add_surface_pixel_format(static_cast<::google::protobuf::uint32>(pf));

    resource_cache->save_resource(response, ipc_package);

    for ( auto const& ext : extensions )
    {
        if (ext.version.empty()) //malformed plugin, ignore
            continue;
        auto e = response->add_extension();
        e->set_name(ext.name);
        for(auto const& v : ext.version)
            e->add_version(v);
    }

    done->Run();
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

    observer->session_create_surface_called(session->name());

    auto params = ms::SurfaceCreationParameters()
        .of_size(request->width(), request->height())
        .of_buffer_usage(static_cast<graphics::BufferUsage>(request->buffer_usage()))
        .of_pixel_format(static_cast<MirPixelFormat>(request->pixel_format()));

    if (request->has_surface_name())
        params.of_name(request->surface_name());

    if (request->has_output_id())
        params.with_output_id(graphics::DisplayConfigurationOutputId(request->output_id()));

    if (request->has_type())
        params.of_type(static_cast<MirWindowType>(request->type()));

    if (request->has_state())
        params.with_state(static_cast<MirWindowState>(request->state()));

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
            params.field = std::remove_reference<decltype(params.field.value())>::type(request->field())

    COPY_IF_SET(min_width);
    COPY_IF_SET(min_height);
    COPY_IF_SET(max_width);
    COPY_IF_SET(max_height);
    COPY_IF_SET(width_inc);
    COPY_IF_SET(height_inc);
    COPY_IF_SET(shell_chrome);
    COPY_IF_SET(confine_pointer);
    COPY_IF_SET(aux_rect_placement_gravity);
    COPY_IF_SET(surface_placement_gravity);
    COPY_IF_SET(placement_hints);
    COPY_IF_SET(aux_rect_placement_offset_x);
    COPY_IF_SET(aux_rect_placement_offset_y);

    #undef COPY_IF_SET

    mf::BufferStreamId buffer_stream_id;
    std::shared_ptr<mf::BufferStream> legacy_stream = nullptr;
    if (request->stream_size() > 0)
    {
        std::vector<msh::StreamSpecification> stream_spec;
        for (auto& stream : request->stream())
        {
            if (stream.has_width() && stream.has_height())
            {
                stream_spec.emplace_back(
                    msh::StreamSpecification{
                        mf::BufferStreamId{stream.id().value()},
                        geom::Displacement{stream.displacement_x(), stream.displacement_y()},
                        geom::Size{stream.width(), stream.height()}});
            }
            else
            {
                stream_spec.emplace_back(
                    msh::StreamSpecification{
                        mf::BufferStreamId{stream.id().value()},
                        geom::Displacement{stream.displacement_x(), stream.displacement_y()},
                        {}});
            }
        }
        params.streams = std::move(stream_spec);
    }
    else
    {
        buffer_stream_id = session->create_buffer_stream(
            {params.size, params.pixel_format, params.buffer_usage});
        legacy_stream = session->get_buffer_stream(buffer_stream_id);
        params.content_id = buffer_stream_id;
    }

    if (request->has_min_aspect())
        params.min_aspect = { request->min_aspect().width(), request->min_aspect().height()};

    if (request->has_max_aspect())
        params.max_aspect = { request->max_aspect().width(), request->max_aspect().height()};

    params.input_shape = extract_input_shape_from(request);

    auto buffering_sender = std::make_shared<mf::ReorderingMessageSender>(message_sender);
    std::shared_ptr<mf::EventSink> sink = sink_factory->create_sink(buffering_sender);

    auto const surf_id = shell->create_surface(session, params, sink);

    auto surface = session->get_surface(surf_id);
    auto const& client_size = surface->client_size();
    response->mutable_id()->set_value(surf_id.as_value());
    response->set_width(client_size.width.as_uint32_t());
    response->set_height(client_size.height.as_uint32_t());

    // TODO: Deprecate
    response->set_pixel_format(request->pixel_format());
    response->set_buffer_usage(request->buffer_usage());

    for (unsigned int i = 0; i < mir_window_attribs; i++)
    {
        auto setting = response->add_attributes();
        
        setting->mutable_surfaceid()->set_value(surf_id.as_value());
        setting->set_attrib(i);
        setting->set_ivalue(shell->get_surface_attribute(session, surf_id, static_cast<MirWindowAttrib>(i)));
    }

    if (legacy_stream)
    {
        response->mutable_buffer_stream()->mutable_id()->set_value(buffer_stream_id.as_value());
        response->mutable_buffer_stream()->set_pixel_format(legacy_stream->pixel_format());
        response->mutable_buffer_stream()->set_buffer_usage(request->buffer_usage());
        legacy_default_stream_map[surf_id] = buffer_stream_id;
    }
    done->Run();
    // ...then uncork the message sender, sending all buffered surface events.
    buffering_sender->uncork();
}

namespace
{
    class AutoSendBuffer : public mg::Buffer
    {
    public:
        AutoSendBuffer(
            std::shared_ptr<mg::Buffer> const& wrapped,
            mir::Executor& executor,
            std::weak_ptr<mf::BufferSink> const& sink)
            : buffer{wrapped},
              executor{executor},
              sink{sink}
        {
        }
        ~AutoSendBuffer()
        {
            executor.spawn(
                [maybe_sink = sink, maybe_to_send = std::weak_ptr<mg::Buffer>(buffer)]()
                {
                    if (auto const live_sink = maybe_sink.lock())
                    {
                        if (auto const& to_send = maybe_to_send.lock())
                            live_sink->update_buffer(*to_send);
                    }
                });
        }

        std::shared_ptr<mir::graphics::NativeBuffer> native_buffer_handle() const override
        {
            return buffer->native_buffer_handle();
        }

        mir::graphics::BufferID id() const override
        {
            return buffer->id();
        }

        mir::geometry::Size size() const override
        {
            return buffer->size();
        }

        MirPixelFormat pixel_format() const override
        {
            return buffer->pixel_format();
        }

        mir::graphics::NativeBufferBase *native_buffer_base() override
        {
            return buffer->native_buffer_base();
        }

    private:
        std::shared_ptr<mg::Buffer> buffer;
        mir::Executor& executor;
        std::weak_ptr<mf::BufferSink> const sink;
    };

}

void mf::SessionMediator::submit_buffer(
    mir::protobuf::BufferRequest const* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    auto const session = weak_session.lock();
    if (!session) BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));
    observer->session_submit_buffer_called(session->name());
    
    mf::BufferStreamId const stream_id{request->id().value()};
    mg::BufferID const buffer_id{static_cast<uint32_t>(request->buffer().buffer_id())};
    auto stream = session->get_buffer_stream(stream_id);

    mfd::ProtobufBufferPacker request_msg{const_cast<mir::protobuf::Buffer*>(&request->buffer())};
    auto b = buffer_cache.at(buffer_id);
    ipc_operations->unpack_buffer(request_msg, *b);

    stream->submit_buffer(std::make_shared<AutoSendBuffer>(b, executor, event_sink));

    done->Run();
}

namespace
{
bool validate_buffer_request(mir::protobuf::BufferStreamParameters const& req)
{
    // A valid buffer request has either flags & native_format set
    // or buffer_usage & pixel_format set, not both.
    return
        (
            req.has_pixel_format() &&
            req.has_buffer_usage() &&
            !req.has_native_format() &&
            !req.has_flags()
        ) ||
        (
            !req.has_pixel_format() &&
            !req.has_buffer_usage() &&
            req.has_native_format() &&
            req.has_flags()
        );
}
}

void mf::SessionMediator::allocate_buffers(
    mir::protobuf::BufferAllocation const* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();
    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    observer->session_allocate_buffers_called(session->name());
    for (auto i = 0; i < request->buffer_requests().size(); i++)
    {
        auto const& req = request->buffer_requests(i);
        std::shared_ptr<mg::Buffer> buffer;
        try
        {
            if (!validate_buffer_request(req))
            {
                BOOST_THROW_EXCEPTION(std::logic_error("Invalid buffer request"));
            }

            if (req.has_flags() && req.has_native_format())
            {
                buffer = allocator->alloc_buffer(
                    {req.width(), req.height()},
                    req.native_format(),
                    req.flags());
            }
            else
            {
                auto const usage = static_cast<mg::BufferUsage>(req.buffer_usage());
                geom::Size const size{req.width(), req.height()};
                auto const pf = static_cast<MirPixelFormat>(req.pixel_format());
                if (usage == mg::BufferUsage::software)
                {
                    buffer = allocator->alloc_software_buffer(size, pf);
                }
                else
                {
                    //legacy route, server-selected pf and usage
                    buffer =
                        allocator->alloc_buffer(mg::BufferProperties{size, pf, mg::BufferUsage::hardware});
                }
            }

            if (request->has_id())
            {
                auto const stream_id = mf::BufferStreamId{request->id().value()};
                // We don't need the stream, but we *do* need to know it exists
                auto stream = session->get_buffer_stream(stream_id);
                stream_associated_buffers.insert(std::make_pair(stream_id, buffer->id()));
            }

            // TODO: Throw if insert fails (duplicate ID)?
            buffer_cache.insert(std::make_pair(buffer->id(), buffer));
            event_sink->add_buffer(*buffer);
        }
        catch (std::exception const& err)
        {
            event_sink->error_buffer(
                geom::Size{req.width(), req.height()},
                static_cast<MirPixelFormat>(req.pixel_format()),
                err.what());
        }
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

    observer->session_release_buffers_called(session->name());

    std::vector<mg::BufferID> to_release(request->buffers().size());
    std::transform(
        request->buffers().begin(),
        request->buffers().end(),
        to_release.begin(),
        [](auto buffer)
        {
            return mg::BufferID{static_cast<uint32_t>(buffer.buffer_id())};
        });

    if (request->has_id())
    {
        auto const stream_id = mf::BufferStreamId{request->id().value()};

        auto const associated_range = stream_associated_buffers.equal_range(stream_id);
        for (auto match = associated_range.first; match != associated_range.second;)
        {
            auto const current = match;
            ++match;
            if (std::find(to_release.begin(), to_release.end(), current->second) != to_release.end())
            {
                stream_associated_buffers.erase(current);
            }
        }
    }
    for (auto const& buffer_id : to_release)
    {
        buffer_cache.erase(buffer_id);
    }
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

    observer->session_release_surface_called(session->name());

    auto const id = SurfaceId(request->value());

    shell->destroy_surface(session, id);

    auto it = legacy_default_stream_map.find(id);
    if (it != legacy_default_stream_map.end())
    {
        session->destroy_buffer_stream(it->second);
        legacy_default_stream_map.erase(it);
    }

    // TODO: We rely on this sending responses synchronously.
    done->Run();
}

void mf::SessionMediator::disconnect(
    const mir::protobuf::Void* /*request*/,
    mir::protobuf::Void* /*response*/,
    google::protobuf::Closure* done)
{
    {
        auto session = weak_session.lock();

        if (session.get() == nullptr)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

        observer->session_disconnect_called(session->name());

        shell->close_session(session);
        destroy_screencast_sessions();
    }
    weak_session.reset();

    done->Run();
}

void mf::SessionMediator::configure_surface(
    const mir::protobuf::SurfaceSetting* request,
    mir::protobuf::SurfaceSetting* response,
    google::protobuf::Closure* done)
{
    auto attrib = static_cast<MirWindowAttrib>(request->attrib());

    // Required response fields:
    response->mutable_surfaceid()->CopyFrom(request->surfaceid());
    response->set_attrib(attrib);

    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    observer->session_configure_surface_called(session->name());

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
    mods.name = std::remove_reference<decltype(mods.name.value())>::type(surface_specification.name())

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
    COPY_IF_SET(aux_rect_placement_gravity);
    COPY_IF_SET(surface_placement_gravity);
    COPY_IF_SET(placement_hints);
    COPY_IF_SET(aux_rect_placement_offset_x);
    COPY_IF_SET(aux_rect_placement_offset_y);
    COPY_IF_SET(edge_attachment);
    COPY_IF_SET(min_width);
    COPY_IF_SET(min_height);
    COPY_IF_SET(max_width);
    COPY_IF_SET(max_height);
    COPY_IF_SET(width_inc);
    COPY_IF_SET(height_inc);
    COPY_IF_SET(shell_chrome);
    COPY_IF_SET(confine_pointer);
    // min_aspect is a special case (below)
    // max_aspect is a special case (below)

#undef COPY_IF_SET
    if (surface_specification.stream_size() > 0)
    {
        std::vector<msh::StreamSpecification> stream_spec;
        for (auto& stream : surface_specification.stream())
        {
            if (stream.has_width() && stream.has_height())
            {
                stream_spec.emplace_back(
                    msh::StreamSpecification{
                        mf::BufferStreamId{stream.id().value()},
                        geom::Displacement{stream.displacement_x(), stream.displacement_y()},
                        geom::Size{stream.width(), stream.height()}});
            }
            else
            {
                stream_spec.emplace_back(
                    msh::StreamSpecification{
                        mf::BufferStreamId{stream.id().value()},
                        geom::Displacement{stream.displacement_x(), stream.displacement_y()},
                        {}});
            }
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

    if (surface_specification.has_cursor_name())
    {
        if (surface_specification.cursor_name() == mir_disabled_cursor_name)
        {
            mods.cursor_image = nullptr;
        }
        else
        {
            mods.cursor_image = cursor_images->image(surface_specification.cursor_name(), mi::default_cursor_size);
        }
    }

    if (surface_specification.has_cursor_id() &&
        surface_specification.has_hotspot_x() &&
        surface_specification.has_hotspot_y())
    {
        mf::BufferStreamId id{surface_specification.cursor_id().value()};
        auto stream = session->get_buffer_stream(id);
        mods.stream_cursor = msh::StreamCursor{
            id, geom::Displacement{surface_specification.hotspot_x(), surface_specification.hotspot_y()} };
    }

    if (surface_specification.input_shape_size() > 0)
        mods.input_shape = extract_input_shape_from(&surface_specification);

    auto const id = mf::SurfaceId(request->surface_id().value());

    shell->modify_surface(session, id, mods);

    done->Run();
}

void mf::SessionMediator::configure_display(
    ::mir::protobuf::DisplayConfiguration const* request,
    ::mir::protobuf::DisplayConfiguration* response,
    ::google::protobuf::Closure* done)
{
    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    observer->session_configure_display_called(session->name());

    auto const config = unpack_and_sanitize_display_configuration(request);
    display_changer->configure(session, config);

    auto display_config = display_changer->base_configuration();
    mfd::pack_protobuf_display_configuration(*response, *display_config);

    done->Run();
}

void mf::SessionMediator::remove_session_configuration(
    ::mir::protobuf::Void const* /*request*/,
    ::mir::protobuf::Void* /*response*/,
    ::google::protobuf::Closure* done)
{
    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    display_changer->remove_session_configuration(session);

    done->Run();
}

void mf::SessionMediator::set_base_display_configuration(
    mir::protobuf::DisplayConfiguration const* request,
    mir::protobuf::Void* /*response*/,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    observer->session_set_base_display_configuration_called(session->name());

    auto const config = unpack_and_sanitize_display_configuration(request);
    display_changer->set_base_configuration(config);

    done->Run();
}

void mf::SessionMediator::preview_base_display_configuration(
    mir::protobuf::PreviewConfiguration const* request,
    mir::protobuf::Void* /*response*/,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    observer->session_preview_base_display_configuration_called(session->name());

    auto const config = unpack_and_sanitize_display_configuration(&request->configuration());
    display_changer->preview_base_configuration(
        weak_session,
        config,
        std::chrono::seconds{request->timeout()});

    done->Run();
}

void mf::SessionMediator::confirm_base_display_configuration(
    mir::protobuf::DisplayConfiguration const* request,
    mir::protobuf::Void* /*response*/,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    observer->session_confirm_base_display_configuration_called(session->name());

    auto const config = unpack_and_sanitize_display_configuration(request);

    display_changer->confirm_base_configuration(session, config);

    done->Run();
}

void mf::SessionMediator::cancel_base_display_configuration_preview(
    mir::protobuf::Void const* /*request*/,
    mir::protobuf::Void* /*response*/,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    display_changer->cancel_base_configuration_preview(session);

    done->Run();
}

void mf::SessionMediator::create_screencast(
    const mir::protobuf::ScreencastParameters* parameters,
    mir::protobuf::Screencast* protobuf_screencast,
    google::protobuf::Closure* done)
{
    auto const msg_type = mg::BufferIpcMsgType::full_msg;

    geom::Rectangle const region{
        {parameters->region().left(), parameters->region().top()},
        {parameters->region().width(), parameters->region().height()}
    };
    geom::Size const size{parameters->width(), parameters->height()};
    MirPixelFormat const pixel_format = static_cast<MirPixelFormat>(parameters->pixel_format());

    int nbuffers = 1;
    if (parameters->has_num_buffers())
        nbuffers = parameters->num_buffers();

    MirMirrorMode mirror_mode = mir_mirror_mode_none;
    if (parameters->has_mirror_mode())
        mirror_mode = static_cast<MirMirrorMode>(parameters->mirror_mode());

    auto screencast_session_id = screencast->create_session(region, size, pixel_format, nbuffers, mirror_mode);

    if (nbuffers > 0)
    {
        auto buffer = screencast->capture(screencast_session_id);
        screencast_buffer_tracker.track_buffer(screencast_session_id, buffer.get());
        pack_protobuf_buffer(
            *protobuf_screencast->mutable_buffer_stream()->mutable_buffer(),
            buffer.get(),
            msg_type);
    }

    protobuf_screencast->mutable_screencast_id()->set_value(
        screencast_session_id.as_value());
    protobuf_screencast->mutable_buffer_stream()->set_pixel_format(pixel_format);
    protobuf_screencast->mutable_buffer_stream()->mutable_id()->set_value(
        screencast_session_id.as_value());
    protobuf_screencast->mutable_buffer_stream()->set_pixel_format(pixel_format);

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
    screencast_buffer_tracker.remove_session(screencast_session_id);
    done->Run();
}

void mf::SessionMediator::screencast_buffer(
    const mir::protobuf::ScreencastId* protobuf_screencast_id,
    mir::protobuf::Buffer* protobuf_buffer,
    google::protobuf::Closure* done)
{
    ScreencastSessionId const screencast_session_id{
        protobuf_screencast_id->value()};

    auto buffer = screencast->capture(screencast_session_id);
    bool const already_tracked = screencast_buffer_tracker.track_buffer(screencast_session_id, buffer.get());
    auto const msg_type = already_tracked ?
        mg::BufferIpcMsgType::update_msg : mg::BufferIpcMsgType::full_msg;
    pack_protobuf_buffer(*protobuf_buffer,
                         buffer.get(),
                         msg_type);

    done->Run();
}

void mf::SessionMediator::screencast_to_buffer(
    mir::protobuf::ScreencastRequest const* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();
    ScreencastSessionId const screencast_session_id{request->id().value()};
    auto buffer = buffer_cache.at(mg::BufferID{request->buffer_id()});
    screencast->capture(screencast_session_id, buffer);
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

    observer->session_create_buffer_stream_called(session->name());
    
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
    done->Run();
}

void mf::SessionMediator::release_buffer_stream(
    const mir::protobuf::BufferStreamId* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    observer->session_release_buffer_stream_called(session->name());

    auto const id = BufferStreamId(request->value());

    session->destroy_buffer_stream(id);

    auto const associated_range = stream_associated_buffers.equal_range(id) ;
    for (auto match = associated_range.first; match != associated_range.second; ++match)
    {
        buffer_cache.erase(match->second);
    }
    stream_associated_buffers.erase(id);

    done->Run();
}


auto mf::SessionMediator::prompt_session_connect_handler(detail::PromptSessionId prompt_session_id) const
-> std::function<void(std::shared_ptr<mf::Session> const&)>
{
    return [this, prompt_session_id](std::shared_ptr<mf::Session> const& session)
    {
        auto prompt_session = prompt_sessions.fetch(prompt_session_id);
        if (prompt_session.get() == nullptr)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid prompt session"));

        shell->add_prompt_provider_for(prompt_session, session);
    };
}

void mf::SessionMediator::configure_cursor(
    mir::protobuf::CursorSetting const* cursor_request,
    mir::protobuf::Void* /* void_response */,
    google::protobuf::Closure* done)
{
    auto session = weak_session.lock();

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    observer->session_configure_surface_cursor_called(session->name());

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

    auto const connect_handler = prompt_session_connect_handler(detail::PromptSessionId(parameters->prompt_session_id()));

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
    for (auto const request_fd : request->fd())
    {
        platform_request.fds.emplace_back(request_fd);
    }

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
    ::mir::protobuf::PromptSession* response,
    ::google::protobuf::Closure* done)
{
    auto const session = weak_session.lock();

    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    ms::PromptSessionCreationParameters parameters;
    parameters.application_pid = request->application_pid();

    observer->session_start_prompt_session_called(session->name(), parameters.application_pid);

    response->set_id(prompt_sessions.insert(shell->start_prompt_session_for(session, parameters)).as_value());

    done->Run();
}

void mf::SessionMediator::stop_prompt_session(
    protobuf::PromptSession const* request,
    ::mir::protobuf::Void*,
    ::google::protobuf::Closure* done)
{
    auto const session = weak_session.lock();

    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    detail::PromptSessionId const id{request->id()};
    auto const prompt_session = prompt_sessions.fetch(id);

    if (!prompt_session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid prompt session"));

    prompt_sessions.remove(id);

    observer->session_stop_prompt_session_called(session->name());

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

void mir::frontend::SessionMediator::request_operation(
    mir::protobuf::RequestWithAuthority const* request,
    mir::protobuf::Void*, google::protobuf::Closure* done)
{
    auto const session = weak_session.lock();
    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    auto const cookie     = request->authority();
    auto const surface_id = request->surface_id();

    auto cookie_string = cookie.cookie();

    std::vector<uint8_t> cookie_bytes(cookie_string.begin(), cookie_string.end());
    auto const cookie_ptr = cookie_authority->make_cookie(cookie_bytes);

    switch (request->operation())
    {
    case mir::protobuf::RequestOperation::START_DRAG_AND_DROP:
        shell->request_operation(
            session, mf::SurfaceId{surface_id.value()},
            cookie_ptr->timestamp(),
            Shell::UserRequest::drag_and_drop);
        break;

    case mir::protobuf::RequestOperation::MAKE_ACTIVE:
        shell->request_operation(
            session, mf::SurfaceId{surface_id.value()},
            cookie_ptr->timestamp(),
            Shell::UserRequest::activate);
        break;

    case mir::protobuf::RequestOperation::USER_MOVE:
        shell->request_operation(
            session, mf::SurfaceId{surface_id.value()},
            cookie_ptr->timestamp(),
            Shell::UserRequest::move);
        break;

    case mir::protobuf::RequestOperation::USER_RESIZE:
        shell->request_operation(
            session, mf::SurfaceId{surface_id.value()},
            cookie_ptr->timestamp(),
            Shell::UserRequest::resize,
            request->hint());
        break;

    default:
        break;
    }

    done->Run();
}

void mf::SessionMediator::apply_input_configuration(
    mir::protobuf::InputConfigurationRequest const* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    auto const session = weak_session.lock();
    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    auto conf = mi::deserialize_input_config(request->input_configuration());
    input_changer->configure(session, std::move(conf));

    done->Run();
}

void mf::SessionMediator::set_base_input_configuration(
    mir::protobuf::InputConfigurationRequest const* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    auto const session = weak_session.lock();
    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    auto conf = mi::deserialize_input_config(request->input_configuration());
    input_changer->set_base_configuration(std::move(conf));

    done->Run();
}

std::shared_ptr<mg::DisplayConfiguration>
mf::SessionMediator::unpack_and_sanitize_display_configuration(
    mir::protobuf::DisplayConfiguration const* protobuf_config)
{
    auto config = display_changer->base_configuration();

    config->for_each_output([&](mg::UserDisplayConfigurationOutput& dest){
        unsigned id = dest.id.as_value();
        int n = 0;
        for (; n < protobuf_config->display_output_size(); ++n)
        {
            if (protobuf_config->display_output(n).output_id() == id)
                break;
        }
        if (n >= protobuf_config->display_output_size())
            return;

        auto& src = protobuf_config->display_output(n);
        dest.used = src.used();
        dest.top_left = geom::Point{src.position_x(),
                src.position_y()};
        dest.current_mode_index = src.current_mode();
        dest.current_format =
                static_cast<MirPixelFormat>(src.current_format());
        dest.power_mode = static_cast<MirPowerMode>(src.power_mode());
        dest.orientation = static_cast<MirOrientation>(src.orientation());
        dest.scale = src.scale_factor();

        dest.gamma = {convert_string_to_gamma_curve(src.gamma_red()),
                      convert_string_to_gamma_curve(src.gamma_green()),
                      convert_string_to_gamma_curve(src.gamma_blue())};

        if (src.has_custom_logical_size())
        {
            if (!src.custom_logical_size())
            {   // User has explicitly removed logical size customization
                if (dest.custom_logical_size.is_set())
                    (void)dest.custom_logical_size.consume();
            }
            else if (src.has_logical_width() && src.has_logical_height())
            {   // User has explicitly set a logical size customization
                dest.custom_logical_size = {src.logical_width(),
                                            src.logical_height()};
            }
        }
    });

    return config;
}

void mf::SessionMediator::destroy_screencast_sessions()
{
    std::vector<ScreencastSessionId> ids_to_untrack;
    screencast_buffer_tracker.for_each_session([this, &ids_to_untrack](ScreencastSessionId id)
    {
        screencast->destroy_session(id);
        ids_to_untrack.push_back(id);
    });

    for (auto const& id : ids_to_untrack)
        screencast_buffer_tracker.remove_session(id);
}

auto mf::detail::PromptSessionStore::insert(std::shared_ptr<PromptSession> const& session) -> PromptSessionId
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    auto const id = PromptSessionId{++next_id};
    sessions[id] = session;
    return id;
}

auto mf::detail::PromptSessionStore::fetch(PromptSessionId session) const
-> std::shared_ptr<PromptSession>
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    return sessions[session].lock();
}

void mf::detail::PromptSessionStore::remove(PromptSessionId session)
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    sessions.erase(session);
}
