/*
 * Copyright © 2014, 2016 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_client_host_connection.h"
#include "host_surface.h"
#include "host_stream.h"
#include "host_chain.h"
#include "host_surface_spec.h"
#include "host_buffer.h"
#include "native_buffer.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_extension_core.h"
#include "mir_toolkit/extensions/mesa_drm_auth.h"
#include "mir_toolkit/extensions/set_gbm_device.h"
#include "mir_toolkit/mir_buffer.h"
#include "mir_toolkit/mesa/platform_operation.h"
#include "mir_toolkit/mir_extension_core.h"
#include "mir_toolkit/mir_buffer_private.h"
#include "mir_toolkit/mir_presentation_chain.h"
#include "mir_toolkit/rs/mir_render_surface.h"
#include "mir_toolkit/extensions/fenced_buffers.h"
#include "mir_toolkit/extensions/android_buffer.h"
#include "mir_toolkit/extensions/gbm_buffer.h"
#include "mir_toolkit/extensions/graphics_module.h"
#include "mir_toolkit/extensions/hardware_buffer_stream.h"
#include "mir/raii.h"
#include "mir/graphics/platform_operation_message.h"
#include "mir/graphics/cursor_image.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/input/device.h"
#include "mir/input/device_capability.h"
#include "mir/input/mir_pointer_config.h"
#include "mir/input/mir_touchpad_config.h"
#include "mir/input/input_device_observer.h"
#include "mir/frontend/event_sink.h"
#include "mir/server_action_queue.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>

#include <algorithm>
#include <stdexcept>
#include <condition_variable>

#include <cstring>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace mi = mir::input;
namespace mf = mir::frontend;

namespace
{
mgn::UniqueInputConfig make_input_config(MirConnection* con)
{
    return mgn::UniqueInputConfig(mir_connection_create_input_config(con), mir_input_config_release);
}

void display_config_callback_thunk(MirConnection* /*connection*/, void* context)
{
    (*static_cast<std::function<void()>*>(context))();
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
class MirClientHostSurface : public mgn::HostSurface
{
public:
    MirClientHostSurface(
        MirConnection* mir_connection,
        MirWindowSpec* spec)
        : mir_connection(mir_connection),
          mir_surface{
              mir_create_window_sync(spec)}
    {
        if (!mir_window_is_valid(mir_surface))
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error(mir_window_get_error_message(mir_surface)));
        }
    }

    ~MirClientHostSurface()
    {
        if (cursor) mir_buffer_stream_release_sync(cursor);
        mir_window_release_sync(mir_surface);
    }

    void apply_spec(mgn::HostSurfaceSpec& spec) override
    {
        mir_window_apply_spec(mir_surface, spec.handle());
    }

    EGLNativeWindowType egl_native_window() override
    {
        return reinterpret_cast<EGLNativeWindowType>(
            mir_buffer_stream_get_egl_native_window(mir_window_get_buffer_stream(mir_surface)));
    }

    void set_event_handler(MirWindowEventCallback cb, void* context) override
    {
        mir_window_set_event_handler(mir_surface, cb, context);
    }

    void set_cursor_image(mg::CursorImage const& image)
    {
        auto const image_width = image.size().width.as_int();
        auto const image_height = image.size().height.as_int();

        if ((image_width <= 0) || (image_height <= 0))
        {
            hide_cursor();
            return;
        }

        MirGraphicsRegion g;

        if (cursor)
        {
            mir_buffer_stream_get_graphics_region(cursor, &g);

            if (image_width != g.width || image_height != g.height)
            {
                mir_render_surface_release(surface);
                surface = nullptr;
                cursor = nullptr;
            }
        }

        bool const new_cursor{!cursor};

        if (new_cursor)
        {
            surface = mir_connection_create_render_surface_sync(
                mir_connection,
                image_width, image_height);

            cursor = mir_render_surface_get_buffer_stream(
                surface, image_width, image_height, cursor_pixel_format);

            mir_buffer_stream_get_graphics_region(cursor, &g);
        }

        copy_image(g, image);

        mir_buffer_stream_swap_buffers_sync(cursor);

        if (new_cursor || cursor_hotspot != image.hotspot())
        {
            cursor_hotspot = image.hotspot();

            auto conf =
                mir_cursor_configuration_from_render_surface(surface, cursor_hotspot.dx.as_int(), cursor_hotspot.dy.as_int());

            mir_window_configure_cursor(mir_surface, conf);
            mir_cursor_configuration_destroy(conf);
        }
    }

    void hide_cursor()
    {
        if (cursor)
        {
            mir_render_surface_release(surface);
            surface = nullptr;
            cursor = nullptr;
        }

        auto spec = mir_create_window_spec(mir_connection);
        mir_window_spec_set_cursor_name(spec, mir_disabled_cursor_name);
        mir_window_apply_spec(mir_surface, spec);
        mir_window_spec_release(spec);
    }

private:
    MirConnection* const mir_connection;
    MirWindow* const mir_surface;
    MirBufferStream* cursor{nullptr};
    mir::geometry::Displacement cursor_hotspot;
    MirRenderSurface* surface{nullptr};
};

class MirClientHostStream : public mgn::HostStream
{
public:
    MirClientHostStream(MirConnection* connection, mg::BufferProperties const& properties) :
        render_surface(mir_connection_create_render_surface_sync(connection,
            properties.size.width.as_int(), properties.size.height.as_int())),
        stream(get_appropriate_stream(connection, render_surface, properties))
    {
    }

    ~MirClientHostStream()
    {
        mir_render_surface_release(render_surface);
    }
    EGLNativeWindowType egl_native_window() const override
    {
        return reinterpret_cast<EGLNativeWindowType>(mir_buffer_stream_get_egl_native_window(stream));
    }

    MirBufferStream* handle() const override
    {
        return stream;
    }

    MirRenderSurface* rs() const override
    {
        return render_surface;
    }

private:
    MirBufferStream* get_appropriate_stream(
        MirConnection* connection, MirRenderSurface* rs, mg::BufferProperties const& props)
    {
        if (props.usage == mg::BufferUsage::software)
        {
            return mir_render_surface_get_buffer_stream(
                rs, props.size.width.as_int(), props.size.height.as_int(), props.format);
        }
        else
        {
            auto ext = mir_extension_hardware_buffer_stream_v1(connection);
            if (!ext)
                BOOST_THROW_EXCEPTION(std::runtime_error("no hardware streams available on platform"));
            return ext->get_hardware_buffer_stream(
                rs, props.size.width.as_int(), props.size.height.as_int(), props.format);
        }
    }

    MirRenderSurface* const render_surface;
    MirBufferStream* const stream;
};
#pragma GCC diagnostic pop

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
            RecursiveReadLock lock{obj->input_config_callback_mutex};
            if (obj->input_config_callback)
                obj->input_config_callback(make_input_config(connection));
        },
        this);
}

mgn::MirClientHostConnection::~MirClientHostConnection()
{
    mir_connection_release(mir_connection);
}

EGLNativeDisplayType mgn::MirClientHostConnection::egl_native_display()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    return reinterpret_cast<EGLNativeDisplayType>(
        mir_connection_get_egl_native_display(mir_connection));
#pragma GCC diagnostic pop
}

auto mgn::MirClientHostConnection::create_display_config()
    -> std::shared_ptr<MirDisplayConfig>
{
    return std::shared_ptr<MirDisplayConfig>(
        mir_connection_create_display_configuration(mir_connection),
        [] (MirDisplayConfig* c)
        {
            if (c) mir_display_config_release(c);
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
    MirDisplayConfig& display_config)
{
    mir_connection_apply_session_display_config(mir_connection, &display_config);
}

std::shared_ptr<mgn::HostSurface> mgn::MirClientHostConnection::create_surface(
    std::shared_ptr<HostStream> const& stream,
    mir::geometry::Displacement displacement,
    mg::BufferProperties properties,
    char const* name, uint32_t output_id)
{
    std::lock_guard<std::mutex> lg(surfaces_mutex);
    auto spec = mir::raii::deleter_for(
        mir_create_normal_window_spec(
            mir_connection,
            properties.size.width.as_int(),
            properties.size.height.as_int()),
        mir_window_spec_release);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MirBufferUsage usage = (properties.usage == mg::BufferUsage::hardware) ?
        mir_buffer_usage_hardware : mir_buffer_usage_software;

    mir_window_spec_set_pixel_format(spec.get(), properties.format);
    mir_window_spec_set_name(spec.get(), name);
    mir_window_spec_set_buffer_usage(spec.get(), usage);
    mir_window_spec_set_fullscreen_on_output(spec.get(), output_id);
    MirBufferStreamInfo info { stream->handle(), displacement.dx.as_int(), displacement.dy.as_int() };
    mir_window_spec_set_streams(spec.get(), &info, 1);
#pragma GCC diagnostic pop

    auto surf = std::shared_ptr<MirClientHostSurface>(
        new MirClientHostSurface(mir_connection, spec.get()),
        [this](MirClientHostSurface *surf)
        {
            std::lock_guard<std::mutex> lg(surfaces_mutex);
            auto it = std::find(surfaces.begin(), surfaces.end(), surf);
            surfaces.erase(it);
            delete surf;
        });

    if (stored_cursor_image.size().width.as_int() && stored_cursor_image.size().height.as_int())
        surf->set_cursor_image(stored_cursor_image);

    surfaces.push_back(surf.get());
    return surf;
}

mg::PlatformOperationMessage mgn::MirClientHostConnection::platform_operation(
    unsigned int op, mg::PlatformOperationMessage const& request)
{
    mg::PlatformOperationMessage response_msg;
    if (op == MirMesaPlatformOperation::auth_magic)
    {
        auto ext = auth_extension();
        if (!ext.is_set())
            BOOST_THROW_EXCEPTION(std::runtime_error("cannot perform auth_magic, not supported by platform"));
        unsigned int magic_request = 0u;
        if (request.data.size() != sizeof(magic_request))
            BOOST_THROW_EXCEPTION(std::runtime_error("Invalid request message for auth_magic platform operation"));

        std::memcpy(&magic_request, request.data.data(), request.data.size());
        auto magic_response = ext.value()->auth_magic(magic_request);
        response_msg.data.resize(sizeof(magic_response));
        std::memcpy(response_msg.data.data(), &magic_response, sizeof(magic_response));
    }
    else if (op == MirMesaPlatformOperation::auth_fd)
    {
        if (request.data.size() != 0 || request.fds.size() != 0)
            BOOST_THROW_EXCEPTION(std::runtime_error("cannot perform auth_fd, invalid request"));

        auto ext = auth_extension();
        if (!ext.is_set())
            BOOST_THROW_EXCEPTION(std::runtime_error("cannot perform auth_fd, not supported by platform"));
        response_msg.fds.push_back(ext.value()->auth_fd());
    }
    else if (op == MirMesaPlatformOperation::set_gbm_device)
    {
        auto ext = set_gbm_extension();
        if (!ext.is_set())
            BOOST_THROW_EXCEPTION(std::runtime_error("cannot perform set_gbm_extension, not supported by platform"));

        gbm_device* device = nullptr;
        if (request.data.size() != sizeof(device))
            BOOST_THROW_EXCEPTION(std::runtime_error("cannot perform set_gbm_device, invalid request"));
        std::memcpy(&device, request.data.data(), request.data.size());

        int const success_response{0};
        auto const r = reinterpret_cast<char const*>(&success_response);
        response_msg.data.assign(r, r + sizeof(success_response));
    }
    else
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("unrecognized platform operation opcode"));
    }

    return response_msg;
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

    auto ext = mir_extension_graphics_module_v1(mir_connection);
    if (!ext)
        BOOST_THROW_EXCEPTION(std::runtime_error("No graphics_module extension present"));

    ext->graphics_module(mir_connection, &properties);

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
    RecursiveWriteLock lock{input_config_callback_mutex};
    input_config_callback = cb;
}

void mgn::MirClientHostConnection::set_input_event_callback(std::function<void(MirEvent const&, mir::geometry::Rectangle const&)> const& cb)
{
    RecursiveWriteLock lock{event_callback_mutex};
    event_callback = cb;
}

void mgn::MirClientHostConnection::emit_input_event(MirEvent const& event, mir::geometry::Rectangle const& source_frame)
{
    RecursiveReadLock lock{event_callback_mutex};
    if (event_callback)
        event_callback(event, source_frame);
}

std::unique_ptr<mgn::HostStream> mgn::MirClientHostConnection::create_stream(
    mg::BufferProperties const& properties) const
{
    return std::make_unique<MirClientHostStream>(mir_connection, properties);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
struct Chain : mgn::HostChain
{
    Chain(MirConnection* connection) :
        render_surface(mir_connection_create_render_surface_sync(connection, 0, 0)),
        chain(mir_render_surface_get_presentation_chain(render_surface))
    {
    }
    ~Chain()
    {
        mir_render_surface_release(render_surface);
    }

    static void buffer_available(MirBuffer* buffer, void* context)
    {
        auto host_buffer = static_cast<mgn::NativeBuffer*>(context);
        host_buffer->available(buffer);
    }

    void submit_buffer(mgn::NativeBuffer& buffer) override
    {
        mir_presentation_chain_submit_buffer(chain, buffer.client_handle(),
                    buffer_available, &buffer);
    }

    void set_submission_mode(mgn::SubmissionMode mode) override
    {
        if (mode == mgn::SubmissionMode::queueing)
            mir_presentation_chain_set_queueing_mode(chain);
        else
            mir_presentation_chain_set_dropping_mode(chain);
    }

    MirPresentationChain* handle() override
    {
        return chain;
    }

    MirRenderSurface* rs() const override
    {
        return render_surface;
    }

private:
    MirRenderSurface* const render_surface;
    MirPresentationChain* chain;
};
#pragma GCC diagnostic pop

std::unique_ptr<mgn::HostChain> mgn::MirClientHostConnection::create_chain() const
{
    return std::make_unique<Chain>(mir_connection);
}

mgn::GraphicsRegion::GraphicsRegion() :
    handle(nullptr)
{
}

mgn::GraphicsRegion::GraphicsRegion(MirBuffer* handle) :
    handle(handle)
{
    mir_buffer_map(handle, this, &layout);
}

mgn::GraphicsRegion::~GraphicsRegion()
{
    if (handle)
        mir_buffer_unmap(handle);
}

namespace
{
class SurfaceSpec : public mgn::HostSurfaceSpec
{
public:
    SurfaceSpec(MirConnection* connection) :
        spec(mir_create_window_spec(connection))
    {
    }

    ~SurfaceSpec()
    {
        mir_window_spec_release(spec);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    void add_chain(mgn::HostChain& chain, geom::Displacement disp, geom::Size size) override
    {
        mir_window_spec_add_render_surface(
            spec, chain.rs(),
            size.width.as_int(), size.height.as_int(),
            disp.dx.as_int(), disp.dy.as_int());
    }

    void add_stream(mgn::HostStream& stream, geom::Displacement disp, geom::Size size) override
    {
        mir_window_spec_add_render_surface(
            spec, stream.rs(),
            size.width.as_int(), size.height.as_int(),
            disp.dx.as_int(), disp.dy.as_int());
    }
#pragma GCC diagnostic pop

    MirWindowSpec* handle() override
    {
        return spec;
    }
private:
    MirWindowSpec* spec;
};
}

std::shared_ptr<mgn::NativeBuffer> mgn::MirClientHostConnection::create_buffer(
    mg::BufferProperties const& properties)
{
    return std::make_shared<mgn::HostBuffer>(mir_connection, properties);
}

std::shared_ptr<mgn::NativeBuffer> mgn::MirClientHostConnection::create_buffer(
    geom::Size size, MirPixelFormat format)
{
    return std::make_shared<mgn::HostBuffer>(mir_connection, size, format);
}

std::shared_ptr<mgn::NativeBuffer> mgn::MirClientHostConnection::create_buffer(
    geom::Size size, uint32_t native_format, uint32_t native_flags)
{
    if (auto ext = mir_extension_gbm_buffer_v1(mir_connection))
    {
        return std::make_shared<mgn::HostBuffer>(mir_connection, ext, size, native_format, native_flags);
    }

    if (auto ext = mir_extension_android_buffer_v1(mir_connection))
    {
        return std::make_shared<mgn::HostBuffer>(mir_connection, ext, size, native_format, native_flags);
    }

    BOOST_THROW_EXCEPTION(std::runtime_error("could not create hardware buffer"));
}

std::unique_ptr<mgn::HostSurfaceSpec> mgn::MirClientHostConnection::create_surface_spec()
{
    return std::make_unique<SurfaceSpec>(mir_connection);
}

bool mgn::MirClientHostConnection::supports_passthrough(mg::BufferUsage)
{
    return mir_extension_android_buffer_v1(mir_connection) ||
           mir_extension_gbm_buffer_v1(mir_connection);
}

void mgn::MirClientHostConnection::apply_input_configuration(MirInputConfig const* config)
{
    mir_connection_apply_session_input_config(mir_connection, config);
}

std::vector<mir::ExtensionDescription> mgn::MirClientHostConnection::extensions() const
{
    std::vector<ExtensionDescription> result;

    auto enumerator = [](void* context, char const* extension, int version)
        {
            auto result = static_cast<std::vector<ExtensionDescription>*>(context);
            result->push_back(ExtensionDescription{extension, {version}});
        };

    mir_connection_enumerate_extensions(mir_connection, &result, enumerator);

    return result;
}

namespace
{
template<typename T>
struct AuthRequest
{
    std::mutex mut;
    std::condition_variable cv;
    bool set = false;
    T rc;
};

template<typename T>
void cb(T rc, void* context)
{
    auto request = static_cast<AuthRequest<T>*>(context);
    std::unique_lock<decltype(request->mut)> lk(request->mut);
    request->set = true;
    request->rc = rc;
    request->cv.notify_all();
}
void cb_fd(int fd, void* context)
{
    cb<mir::Fd>(mir::Fd(mir::IntOwnedFd{fd}), context);
}
void cb_magic(int response, void* context)
{
    cb<int>(response, context);
}

template<typename T>
T auth(std::function<void(AuthRequest<T>*)> const& f)
{
    auto req = std::make_unique<AuthRequest<T>>();
    f(req.get());
    std::unique_lock<decltype(req->mut)> lk(req->mut);
    req->cv.wait(lk, [&]{ return req->set; });
    return req->rc;
}
}

mir::optional_value<mir::Fd> mgn::MirClientHostConnection::drm_fd()
{
    return {};
}

mir::optional_value<std::shared_ptr<mir::graphics::MesaAuthExtension>>
mgn::MirClientHostConnection::auth_extension()
{
    auto ext = mir_extension_mesa_drm_auth_v1(mir_connection);
    if (!ext)
        return {};

    struct AuthExtension : MesaAuthExtension
    {
        AuthExtension(
            MirConnection* connection,
            MirExtensionMesaDRMAuthV1 const* ext) :
            connection(connection),
            extensions(ext)
        {
        }

        mir::Fd auth_fd() override
        {
            return auth<mir::Fd>([this](auto req){ extensions->drm_auth_fd(connection, cb_fd, req); });
        }

        int auth_magic(unsigned int magic) override
        {
            return auth<int>([this, magic](auto req){ extensions->drm_auth_magic(connection, magic, cb_magic, req); });
        }
    private:
        MirConnection* const connection;
        MirExtensionMesaDRMAuthV1 const * const extensions;
    };
    return { std::make_unique<AuthExtension>(mir_connection, ext) };
}

mir::optional_value<std::shared_ptr<mg::SetGbmExtension>>
mgn::MirClientHostConnection::set_gbm_extension()
{
    auto ext = mir_extension_set_gbm_device_v1(mir_connection);
    if (!ext)
        return {};

    struct SetGbm : SetGbmExtension
    {
        SetGbm(MirExtensionSetGbmDevice const* ext) :
            ext(ext)
        {
        }
        void set_gbm_device(gbm_device* dev) override
        {
            ext->set_gbm_device(dev, ext->context);
        }
        MirExtensionSetGbmDeviceV1 const* const ext;
    };
    return { std::make_unique<SetGbm>(ext) };
}
