/*
 * Copyright © 2014,2016 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_client_host_connection.h"
#include "host_surface.h"
#include "mir_toolkit/mir_client_library.h"

#include "mir/raii.h"
#include "mir/graphics/platform_operation_message.h"
#include "mir/graphics/cursor_image.h"
#include "mir/input/device.h"
#include "mir/input/device_capability.h"
#include "mir/input/pointer_configuration.h"
#include "mir/input/touchpad_configuration.h"
#include "mir/input/input_device_observer.h"
#include "mir/frontend/event_sink.h"
#include "mir/server_action_queue.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>

#include <algorithm>
#include <stdexcept>

#include <cstring>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace mi = mir::input;
namespace mf = mir::frontend;

namespace
{
mgn::UniqueInputConfig make_input_config(MirConnection* con)
{
    return mgn::UniqueInputConfig(mir_connection_create_input_config(con), mir_input_config_destroy);
}

void display_config_callback_thunk(MirConnection* /*connection*/, void* context)
{
    (*static_cast<std::function<void()>*>(context))();
}

void platform_operation_callback(
    MirConnection*, MirPlatformMessage* reply, void* context)
{
    auto reply_ptr = static_cast<MirPlatformMessage**>(context);
    *reply_ptr = reply;
}

static void nested_lifecycle_event_callback_thunk(MirConnection* /*connection*/, MirLifecycleState state, void *context)
{
    msh::HostLifecycleEventListener* listener = static_cast<msh::HostLifecycleEventListener*>(context);
    listener->lifecycle_event_occurred(state);
}

MirPixelFormat const cursor_pixel_format = mir_pixel_format_argb_8888;

void copy_image(MirGraphicsRegion const& g, mg::CursorImage const& image)
{
    assert(g.pixel_format == cursor_pixel_format);

    auto const image_stride = image.size().width.as_int() * MIR_BYTES_PER_PIXEL(cursor_pixel_format);
    auto const image_height = image.size().height.as_int();

    auto dest = g.vaddr;
    auto src  = static_cast<char const*>(image.as_argb_8888());

    for (int row = 0; row != image_height; ++row)
    {
        std::memcpy(dest, src, image_stride);
        dest += g.stride;
        src += image_stride;
    }
}

class MirClientHostSurface : public mgn::HostSurface
{
public:
    MirClientHostSurface(
        MirConnection* mir_connection,
        MirSurfaceSpec* spec)
        : mir_connection(mir_connection),
          mir_surface{
              mir_surface_create_sync(spec)}
    {
        if (!mir_surface_is_valid(mir_surface))
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error(mir_surface_get_error_message(mir_surface)));
        }
    }

    ~MirClientHostSurface()
    {
        if (cursor) mir_buffer_stream_release_sync(cursor);
        mir_surface_release_sync(mir_surface);
    }

    EGLNativeWindowType egl_native_window() override
    {
        return reinterpret_cast<EGLNativeWindowType>(
            mir_buffer_stream_get_egl_native_window(mir_surface_get_buffer_stream(mir_surface)));
    }

    void set_event_handler(mir_surface_event_callback cb, void* context) override
    {
        mir_surface_set_event_handler(mir_surface, cb, context);
    }

    void set_cursor_image(mg::CursorImage const& image)
    {
        auto const image_width = image.size().width.as_int();
        auto const image_height = image.size().height.as_int();

        MirGraphicsRegion g;

        if (cursor)
        {
            mir_buffer_stream_get_graphics_region(cursor, &g);

            if (image_width != g.width || image_height != g.height)
            {
                mir_buffer_stream_release_sync(cursor);
                cursor = nullptr;
            }
        }

        bool const new_cursor{!cursor};

        if (new_cursor)
        {
            cursor = mir_connection_create_buffer_stream_sync(
                mir_connection,
                image_width,
                image_height,
                cursor_pixel_format,
                mir_buffer_usage_software);

            mir_buffer_stream_get_graphics_region(cursor, &g);
        }

        copy_image(g, image);

        mir_buffer_stream_swap_buffers_sync(cursor);

        if (new_cursor || cursor_hotspot != image.hotspot())
        {
            cursor_hotspot = image.hotspot();

            auto conf = mir_cursor_configuration_from_buffer_stream(
                cursor, cursor_hotspot.dx.as_int(), cursor_hotspot.dy.as_int());

            mir_surface_configure_cursor(mir_surface, conf);
            mir_cursor_configuration_destroy(conf);
        }
    }

    void hide_cursor()
    {
        if (cursor) { mir_buffer_stream_release_sync(cursor); cursor = nullptr; }

        auto conf = mir_cursor_configuration_from_name(mir_disabled_cursor_name);
        mir_surface_configure_cursor(mir_surface, conf);
        mir_cursor_configuration_destroy(conf);
    }

private:
    MirConnection* const mir_connection;
    MirSurface* const mir_surface;
    MirBufferStream* cursor{nullptr};
    mir::geometry::Displacement cursor_hotspot;
};

}

void const* mgn::MirClientHostConnection::NestedCursorImage::as_argb_8888() const
{
    return buffer.data();
}

mir::geometry::Size mgn::MirClientHostConnection::NestedCursorImage::size() const
{
    return size_;
}

mir::geometry::Displacement mgn::MirClientHostConnection::NestedCursorImage::hotspot() const
{
    return hotspot_;
}

mgn::MirClientHostConnection::NestedCursorImage::NestedCursorImage(mg::CursorImage const& other)
    : hotspot_(other.hotspot()),
      size_(other.size()),
      buffer(size_.width.as_int() * size_.height.as_int() * MIR_BYTES_PER_PIXEL(mir_pixel_format_argb_8888))
{
    std::memcpy(buffer.data(), other.as_argb_8888(), buffer.size());
}

mgn::MirClientHostConnection::NestedCursorImage& mgn::MirClientHostConnection::NestedCursorImage::operator=(mg::CursorImage const& other)
{
    hotspot_ = other.hotspot();
    size_ = other.size();
    buffer.resize(size_.width.as_int() * size_.height.as_int() * MIR_BYTES_PER_PIXEL(mir_pixel_format_argb_8888));
    std::memcpy(buffer.data(), other.as_argb_8888(), buffer.size());

    return *this;
}

mgn::MirClientHostConnection::MirClientHostConnection(
    std::string const& host_socket,
    std::string const& name,
    std::shared_ptr<msh::HostLifecycleEventListener> const& host_lifecycle_event_listener)
    : mir_connection{mir_connect_sync(host_socket.c_str(), name.c_str())},
      conf_change_callback{[]{}},
      host_lifecycle_event_listener{host_lifecycle_event_listener},
      event_callback{[](MirEvent const&, mir::geometry::Rectangle const&){}}
{
    if (!mir_connection_is_valid(mir_connection))
    {
        std::string const msg =
            "Nested Mir Platform Connection Error: " +
            std::string(mir_connection_get_error_message(mir_connection));

        BOOST_THROW_EXCEPTION(std::runtime_error(msg));
    }

    mir_connection_set_lifecycle_event_callback(
        mir_connection,
        nested_lifecycle_event_callback_thunk,
        std::static_pointer_cast<void>(host_lifecycle_event_listener).get());

    mir_connection_set_input_config_change_callback(
        mir_connection,
        [](MirConnection* connection, void* context)
        {
            auto obj = static_cast<MirClientHostConnection*>(context);
            if (obj->input_config_callback)
                obj->input_config_callback(make_input_config(connection));
        },
        this);
}

mgn::MirClientHostConnection::~MirClientHostConnection()
{
    mir_connection_release(mir_connection);
}

std::vector<int> mgn::MirClientHostConnection::platform_fd_items()
{
    MirPlatformPackage pkg;
    mir_connection_get_platform(mir_connection, &pkg);
    return std::vector<int>(pkg.fd, pkg.fd + pkg.fd_items);
}

EGLNativeDisplayType mgn::MirClientHostConnection::egl_native_display()
{
    return reinterpret_cast<EGLNativeDisplayType>(
        mir_connection_get_egl_native_display(mir_connection));
}

auto mgn::MirClientHostConnection::create_display_config()
    -> std::shared_ptr<MirDisplayConfiguration>
{
    return std::shared_ptr<MirDisplayConfiguration>(
        mir_connection_create_display_config(mir_connection),
        [] (MirDisplayConfiguration* c)
        {
            if (c) mir_display_config_destroy(c);
        });
}

void mgn::MirClientHostConnection::set_display_config_change_callback(
    std::function<void()> const& callback)
{
    mir_connection_set_display_config_change_callback(
        mir_connection,
        &display_config_callback_thunk,
        &(conf_change_callback = callback));
}

void mgn::MirClientHostConnection::apply_display_config(
    MirDisplayConfiguration& display_config)
{
    mir_wait_for(mir_connection_apply_display_config(mir_connection, &display_config));
}

std::shared_ptr<mgn::HostSurface> mgn::MirClientHostConnection::create_surface(
    int width, int height, MirPixelFormat pf, char const* name,
    MirBufferUsage usage, uint32_t output_id)
{
    std::lock_guard<std::mutex> lg(surfaces_mutex);
    auto spec = mir::raii::deleter_for(
        mir_connection_create_spec_for_normal_surface(mir_connection, width, height, pf),
        mir_surface_spec_release);

    mir_surface_spec_set_name(spec.get(), name);
    mir_surface_spec_set_buffer_usage(spec.get(), usage);
    mir_surface_spec_set_fullscreen_on_output(spec.get(), output_id);

    auto surf = std::shared_ptr<MirClientHostSurface>(
        new MirClientHostSurface(mir_connection, spec.get()),
        [this](MirClientHostSurface *surf)
        {
            std::lock_guard<std::mutex> lg(surfaces_mutex);
            auto it = std::find(surfaces.begin(), surfaces.end(), surf);
            surfaces.erase(it);
            delete surf;
        });

    if (stored_cursor_image.size().width.as_int() * stored_cursor_image.size().height.as_int())
        surf->set_cursor_image(stored_cursor_image);

    surfaces.push_back(surf.get());
    return surf;
}

mg::PlatformOperationMessage mgn::MirClientHostConnection::platform_operation(
    unsigned int op, mg::PlatformOperationMessage const& request)
{
    auto const msg = mir::raii::deleter_for(
        mir_platform_message_create(op),
        mir_platform_message_release);

    mir_platform_message_set_data(msg.get(), request.data.data(), request.data.size());
    mir_platform_message_set_fds(msg.get(), request.fds.data(), request.fds.size());

    MirPlatformMessage* raw_reply{nullptr};

    auto const wh = mir_connection_platform_operation(
        mir_connection, msg.get(), platform_operation_callback, &raw_reply);
    mir_wait_for(wh);

    auto const reply = mir::raii::deleter_for(
        raw_reply,
        mir_platform_message_release);

    auto reply_data = mir_platform_message_get_data(reply.get());
    auto reply_fds = mir_platform_message_get_fds(reply.get());

    return PlatformOperationMessage{
        {static_cast<uint8_t const*>(reply_data.data),
         static_cast<uint8_t const*>(reply_data.data) + reply_data.size},
        {reply_fds.fds, reply_fds.fds + reply_fds.num_fds}};
}

void mgn::MirClientHostConnection::set_cursor_image(mg::CursorImage const& image)
{
    std::lock_guard<std::mutex> lg(surfaces_mutex);
    stored_cursor_image = image;
    for (auto s : surfaces)
    {
        auto surface = static_cast<MirClientHostSurface*>(s);
        surface->set_cursor_image(stored_cursor_image);
    }
}

void mgn::MirClientHostConnection::hide_cursor()
{
    std::lock_guard<std::mutex> lg(surfaces_mutex);
    for (auto s : surfaces)
    {
        auto surface = static_cast<MirClientHostSurface*>(s);
        surface->hide_cursor();
    }
}

auto mgn::MirClientHostConnection::graphics_platform_library() -> std::string
{
    MirModuleProperties properties = { nullptr, 0, 0, 0, nullptr };

    mir_connection_get_graphics_module(mir_connection, &properties);

    if (properties.filename == nullptr)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Cannot identify host graphics platform"));
    }

    return properties.filename;
}

mgn::UniqueInputConfig mgn::MirClientHostConnection::create_input_device_config()
{
    return make_input_config(mir_connection);
}

void mgn::MirClientHostConnection::set_input_device_change_callback(std::function<void(UniqueInputConfig)> const& cb)
{
    input_config_callback = cb;
}

void mgn::MirClientHostConnection::set_input_event_callback(std::function<void(MirEvent const&, mir::geometry::Rectangle const&)> const& cb)
{
    event_callback = cb;
}

void mgn::MirClientHostConnection::emit_input_event(MirEvent const& event, mir::geometry::Rectangle const& source_frame)
{
    event_callback(event, source_frame);
}
