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
#include "host_stream.h"
#include "host_chain.h"
#include "host_surface_spec.h"
#include "native_buffer.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_extension_core.h"
#include "mir_toolkit/extensions/mesa_drm_auth.h"
#include "mir_toolkit/extensions/set_gbm_device.h"
#include "mir_toolkit/mir_buffer.h"
#include "mir_toolkit/mir_extension_core.h"
#include "mir_toolkit/mir_buffer_private.h"
#include "mir_toolkit/mir_presentation_chain.h"
#include "mir_toolkit/extensions/fenced_buffers.h"
#include "mir_toolkit/extensions/android_buffer.h"
#include "mir_toolkit/extensions/gbm_buffer.h"
#include "mir/raii.h"
#include "mir/graphics/platform_operation_message.h"
#include "mir/graphics/cursor_image.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/input/device.h"
#include "mir/input/device_capability.h"
#include "mir/input/mir_pointer_configuration.h"
#include "mir/input/mir_touchpad_configuration.h"
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

    void apply_spec(mgn::HostSurfaceSpec& spec) override
    {
        mir_surface_apply_spec(mir_surface, spec.handle());
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

        auto spec = mir_connection_create_spec_for_changes(mir_connection);
        mir_surface_spec_set_cursor_name(spec, mir_disabled_cursor_name);
        mir_surface_apply_spec(mir_surface, spec);
        mir_surface_spec_release(spec);
    }

private:
    MirConnection* const mir_connection;
    MirSurface* const mir_surface;
    MirBufferStream* cursor{nullptr};
    mir::geometry::Displacement cursor_hotspot;
};

class MirClientHostStream : public mgn::HostStream
{
public:
    MirClientHostStream(MirConnection* connection, mg::BufferProperties const& properties) :
        stream(mir_connection_create_buffer_stream_sync(connection,
            properties.size.width.as_int(), properties.size.height.as_int(),
            properties.format,
            (properties.usage == mg::BufferUsage::hardware) ? mir_buffer_usage_hardware : mir_buffer_usage_software))
    {
    }

    ~MirClientHostStream()
    {
        mir_buffer_stream_release_sync(stream);
    }

    EGLNativeWindowType egl_native_window() const override
    {
        return reinterpret_cast<EGLNativeWindowType>(mir_buffer_stream_get_egl_native_window(stream));
    }

    MirBufferStream* handle() const override
    {
        return stream;
    }
private:
    MirBufferStream* const stream;
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
    std::shared_ptr<HostStream> const& stream,
    mir::geometry::Displacement displacement,
    mg::BufferProperties properties,
    char const* name, uint32_t output_id)
{
    std::lock_guard<std::mutex> lg(surfaces_mutex);
    auto spec = mir::raii::deleter_for(
        mir_connection_create_spec_for_normal_surface(
            mir_connection,
            properties.size.width.as_int(),
            properties.size.height.as_int(),
            properties.format),
        mir_surface_spec_release);

    MirBufferUsage usage = (properties.usage == mg::BufferUsage::hardware) ?
        mir_buffer_usage_hardware : mir_buffer_usage_software; 

    mir_surface_spec_set_name(spec.get(), name);
    mir_surface_spec_set_buffer_usage(spec.get(), usage);
    mir_surface_spec_set_fullscreen_on_output(spec.get(), output_id);
    MirBufferStreamInfo info { stream->handle(), displacement.dx.as_int(), displacement.dy.as_int() };
    mir_surface_spec_set_streams(spec.get(), &info, 1);

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

struct Chain : mgn::HostChain
{
    Chain(MirConnection* connection) :
        chain(mir_connection_create_presentation_chain_sync(connection))
    {
    }
    ~Chain()
    {
        mir_presentation_chain_release(chain);
    }

    void submit_buffer(mgn::NativeBuffer& buffer) override
    {
        mir_presentation_chain_submit_buffer(chain, buffer.client_handle());
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
private:
    MirPresentationChain* chain;
};

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
class HostBuffer : public mgn::NativeBuffer
{
public:
    //software
    HostBuffer(MirConnection* mir_connection, geom::Size size, MirPixelFormat format) :
        fence_extensions(mir_extension_fenced_buffers_v1(mir_connection))
    {
        mir_connection_allocate_buffer(
            mir_connection,
            size.width.as_int(),
            size.height.as_int(),
            format,
            mir_buffer_usage_software,
            buffer_available, this);
        std::unique_lock<std::mutex> lk(mut);
        cv.wait(lk, [&]{ return handle; });
        if (!mir_buffer_is_valid(handle))
        {
            mir_buffer_release(handle);
            BOOST_THROW_EXCEPTION(std::runtime_error("could not allocate MirBuffer"));
        }
    }

    HostBuffer(MirConnection* mir_connection,
        MirExtensionGbmBufferV1 const* ext,
        geom::Size size, unsigned int native_pf, unsigned int native_flags) :
        fence_extensions(mir_extension_fenced_buffers_v1(mir_connection))
    {
        ext->allocate_buffer_gbm(
            mir_connection, size.width.as_int(), size.height.as_int(), native_pf, native_flags,
            buffer_available, this);
        std::unique_lock<std::mutex> lk(mut);
        cv.wait(lk, [&]{ return handle; });
        if (!mir_buffer_is_valid(handle))
        {
            mir_buffer_release(handle);
            BOOST_THROW_EXCEPTION(std::runtime_error("could not allocate MirBuffer"));
        }
    }

    HostBuffer(MirConnection* mir_connection,
        MirExtensionAndroidBufferV1 const* ext,
        geom::Size size, unsigned int native_pf, unsigned int native_flags) :
        fence_extensions(mir_extension_fenced_buffers_v1(mir_connection))
    {
        ext->allocate_buffer_android(
            mir_connection, size.width.as_int(), size.height.as_int(), native_pf, native_flags,
            buffer_available, this);
        std::unique_lock<std::mutex> lk(mut);
        cv.wait(lk, [&]{ return handle; });
        if (!mir_buffer_is_valid(handle))
        {
            mir_buffer_release(handle);
            BOOST_THROW_EXCEPTION(std::runtime_error("could not allocate MirBuffer"));
        }
    }

    ~HostBuffer()
    {
        mir_buffer_release(handle);
    }

    void sync(MirBufferAccess access, std::chrono::nanoseconds ns) override
    {
        if (fence_extensions && fence_extensions->wait_for_access)
            fence_extensions->wait_for_access(handle, access, ns.count());
    }

    MirBuffer* client_handle() const override
    {
        return handle;
    }

    std::unique_ptr<mgn::GraphicsRegion> get_graphics_region() override
    {
        return std::make_unique<mgn::GraphicsRegion>(handle);
    }

    geom::Size size() const override
    {
        return { mir_buffer_get_width(handle), mir_buffer_get_height(handle) };
    }

    MirPixelFormat format() const override
    {
        return mir_buffer_get_pixel_format(handle);
    }

    std::tuple<EGLenum, EGLClientBuffer, EGLint*> egl_image_creation_hints() const override
    {
        EGLenum type;
        EGLClientBuffer client_buffer = nullptr;;
        EGLint* attrs = nullptr;
        // TODO: check return value
        mir_buffer_get_egl_image_parameters(handle, &type, &client_buffer, &attrs);
        
        return std::tuple<EGLenum, EGLClientBuffer, EGLint*>{type, client_buffer, attrs};
    }

    static void buffer_available(MirBuffer* buffer, void* context)
    {
        auto host_buffer = static_cast<HostBuffer*>(context);
        host_buffer->available(buffer);
    }

    void available(MirBuffer* buffer)
    {
        std::unique_lock<std::mutex> lk(mut);
        if (!handle)
        {
            handle = buffer;
            cv.notify_all();
        }

        auto g = f;
        lk.unlock();
        if (g)
            g();
    }

    void on_ownership_notification(std::function<void()> const& fn) override
    {
        std::unique_lock<std::mutex> lk(mut);
        f = fn;
    }

    MirBufferPackage* package() const override
    {
       return mir_buffer_get_buffer_package(handle);
    }

    void set_fence(mir::Fd fd) override
    {
        if (fence_extensions && fence_extensions->associate_fence)
            fence_extensions->associate_fence(handle, fd, mir_read_write);
    }

    mir::Fd fence() const override
    {
        if (fence_extensions && fence_extensions->get_fence)
            return mir::Fd{fence_extensions->get_fence(handle)};
        else
            return mir::Fd{mir::Fd::invalid};
    }

private:
    MirExtensionFencedBuffersV1 const* fence_extensions = nullptr;

    std::function<void()> f;
    MirBuffer* handle = nullptr;
    std::mutex mut;
    std::condition_variable cv;
};

class SurfaceSpec : public mgn::HostSurfaceSpec
{
public:
    SurfaceSpec(MirConnection* connection) :
        spec(mir_connection_create_spec_for_changes(connection))
    {
    }

    ~SurfaceSpec()
    {
        mir_surface_spec_release(spec);
    }

    void add_chain(mgn::HostChain& chain, geom::Displacement disp, geom::Size size) override
    {
        mir_surface_spec_add_presentation_chain(
            spec, size.width.as_int(), size.height.as_int(),
            disp.dx.as_int(), disp.dy.as_int(), chain.handle());
    }

    void add_stream(mgn::HostStream& stream, geom::Displacement disp, geom::Size size) override
    {
        mir_surface_spec_add_buffer_stream(spec,
            disp.dx.as_int(), disp.dy.as_int(),
            size.width.as_int(), size.height.as_int(), stream.handle());
    }

    MirSurfaceSpec* handle() override
    {
        return spec;
    }
private:
    MirSurfaceSpec* spec;
};
}

std::shared_ptr<mgn::NativeBuffer> mgn::MirClientHostConnection::create_buffer(
    geom::Size size, MirPixelFormat format)
{
    return std::make_shared<HostBuffer>(mir_connection, size, format);
}

std::shared_ptr<mgn::NativeBuffer> mgn::MirClientHostConnection::create_buffer(
    mg::BufferRequestMessage const& request)
{
    if (auto ext = mir_extension_gbm_buffer_v1(mir_connection))
    {
        return std::make_shared<HostBuffer>(
            mir_connection, ext, request.size, request.native_format, request.native_flags);
    }

    if (auto ext = mir_extension_android_buffer_v1(mir_connection))
    {
        return std::make_shared<HostBuffer>(
            mir_connection, ext, request.size, request.native_format, request.native_flags);
    }

    BOOST_THROW_EXCEPTION(std::runtime_error("could not create hardware buffer"));
}

std::unique_ptr<mgn::HostSurfaceSpec> mgn::MirClientHostConnection::create_surface_spec()
{
    return std::make_unique<SurfaceSpec>(mir_connection);
}

bool mgn::MirClientHostConnection::supports_passthrough()
{
    auto buffer = create_buffer(geom::Size{1, 1} , mir_pixel_format_abgr_8888);
    auto hints = buffer->egl_image_creation_hints();
    if (std::get<1>(hints) == nullptr && std::get<2>(hints) == nullptr)
        return false;
    return true;
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
