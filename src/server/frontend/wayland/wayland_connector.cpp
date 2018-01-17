/*
 * Copyright © 2015-2018 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "wayland_connector.h"

#include "core_generated_interfaces.h"
#include "xdg_shell_generated_interfaces.h"

#include "mir/frontend/shell.h"
#include "mir/frontend/surface.h"
#include "mir/frontend/session_credentials.h"
#include "mir/frontend/session_authorizer.h"

#include "mir/compositor/buffer_stream.h"

#include "mir/frontend/session.h"
#include "mir/frontend/event_sink.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/surface.h"

#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/wayland_allocator.h"

#include "mir/renderer/gl/texture_target.h"
#include "mir/frontend/buffer_stream_id.h"
#include "mir/frontend/display_changer.h"

#include "mir/executor.h"

#include "mir/client/event.h"

#include "mir/input/device.h"
#include "mir/input/mir_keyboard_config.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/input_device_observer.h"

#include <system_error>
#include <sys/eventfd.h>
#include <wayland-server-core.h>
#include <unordered_map>
#include <boost/throw_exception.hpp>

#include <future>
#include <functional>
#include <type_traits>

#include <xkbcommon/xkbcommon.h>
#include <linux/input.h>
#include <algorithm>
#include <iostream>
#include <mir/log.h>
#include <cstring>
#include <deque>
#include MIR_SERVER_GL_H
#include MIR_SERVER_GLEXT_H

#include "mir/fd.h"
#include "../../../platforms/common/server/shm_buffer.h"

#include <sys/stat.h>
#include <sys/socket.h>
#include "mir/anonymous_shm_file.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mcl = mir::client;
namespace mi = mir::input;

namespace mir
{
namespace frontend
{

class WaylandEventSink : public mf::EventSink
{
public:
    WaylandEventSink(std::function<void(MirLifecycleState)> const& lifecycle_handler)
        : lifecycle_handler{lifecycle_handler}
    {
    }

    void handle_event(const MirEvent& e) override;
    void handle_lifecycle_event(MirLifecycleState state) override;
    void handle_display_config_change(graphics::DisplayConfiguration const& config) override;
    void send_ping(int32_t serial) override;
    void send_buffer(BufferStreamId id, graphics::Buffer& buffer, graphics::BufferIpcMsgType type) override;
    void handle_input_config_change(MirInputConfig const&) override {}
    void handle_error(ClientVisibleError const&) override {}

    void add_buffer(graphics::Buffer&) override {}
    void error_buffer(geometry::Size, MirPixelFormat, std::string const& ) override {}
    void update_buffer(graphics::Buffer&) override {}

private:
    std::function<void(MirLifecycleState)> const lifecycle_handler;
};

namespace
{
bool get_gl_pixel_format(
    MirPixelFormat mir_format,
    GLenum& gl_format,
    GLenum& gl_type)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    GLenum const argb = GL_BGRA_EXT;
    GLenum const abgr = GL_RGBA;
#elif __BYTE_ORDER == __BIG_ENDIAN
    // TODO: Big endian support
GLenum const argb = GL_INVALID_ENUM;
GLenum const abgr = GL_INVALID_ENUM;
//GLenum const rgba = GL_RGBA;
//GLenum const bgra = GL_BGRA_EXT;
#endif

    static const struct
    {
        MirPixelFormat mir_format;
        GLenum gl_format, gl_type;
    } mapping[mir_pixel_formats] =
        {
            {mir_pixel_format_invalid,   GL_INVALID_ENUM, GL_INVALID_ENUM},
            {mir_pixel_format_abgr_8888, abgr,            GL_UNSIGNED_BYTE},
            {mir_pixel_format_xbgr_8888, abgr,            GL_UNSIGNED_BYTE},
            {mir_pixel_format_argb_8888, argb,            GL_UNSIGNED_BYTE},
            {mir_pixel_format_xrgb_8888, argb,            GL_UNSIGNED_BYTE},
            {mir_pixel_format_bgr_888,   GL_INVALID_ENUM, GL_INVALID_ENUM},
            {mir_pixel_format_rgb_888,   GL_RGB,          GL_UNSIGNED_BYTE},
            {mir_pixel_format_rgb_565,   GL_RGB,          GL_UNSIGNED_SHORT_5_6_5},
            {mir_pixel_format_rgba_5551, GL_RGBA,         GL_UNSIGNED_SHORT_5_5_5_1},
            {mir_pixel_format_rgba_4444, GL_RGBA,         GL_UNSIGNED_SHORT_4_4_4_4},
        };

    if (mir_format > mir_pixel_format_invalid &&
        mir_format < mir_pixel_formats &&
        mapping[mir_format].mir_format == mir_format) // just a sanity check
    {
        gl_format = mapping[mir_format].gl_format;
        gl_type = mapping[mir_format].gl_type;
    }
    else
    {
        gl_format = GL_INVALID_ENUM;
        gl_type = GL_INVALID_ENUM;
    }

    return gl_format != GL_INVALID_ENUM && gl_type != GL_INVALID_ENUM;
}


struct ClientPrivate
{
    ClientPrivate(std::shared_ptr<mf::Session> const& session, mf::Shell& shell)
        : session{session},
          shell{&shell}
    {
    }

    ~ClientPrivate()
    {
        shell->close_session(session);
        /*
         * This ensures that further calls to
         * wl_client_get_destroy_listener(client, &cleanup_private)
         * - and hence session_for_client(client) - return nullptr.
         */
        wl_list_remove(&destroy_listener.link);
    }

    wl_listener destroy_listener;
    std::shared_ptr<mf::Session> const session;
    /*
     * This shell is owned by the ClientSessionConstructor, which outlives all clients.
     */
    mf::Shell* const shell;
};

static_assert(
    std::is_standard_layout<ClientPrivate>::value,
    "ClientPrivate must be standard layout for wl_container_of to be defined behaviour");


ClientPrivate* private_from_listener(wl_listener* listener)
{
    ClientPrivate* userdata;
    return wl_container_of(listener, userdata, destroy_listener);
}

void cleanup_private(wl_listener* listener, void* /*data*/)
{
    delete private_from_listener(listener);
}

std::shared_ptr<mf::Session> session_for_client(wl_client* client)
{
    auto listener = wl_client_get_destroy_listener(client, &cleanup_private);

    if (listener)
        return private_from_listener(listener)->session;

    return nullptr;
}

std::shared_ptr<ms::Surface> get_surface_for_id(std::shared_ptr<mf::Session> const& session, mf::SurfaceId surface_id)
{
    return std::dynamic_pointer_cast<ms::Surface>(session->get_surface(surface_id));
}

struct ClientSessionConstructor
{
    ClientSessionConstructor(std::shared_ptr<mf::Shell> const& shell,
                             std::shared_ptr<mf::SessionAuthorizer> const& session_authorizer)
        : shell{shell},
          session_authorizer{session_authorizer}
    {
    }

    wl_listener construction_listener;
    wl_listener destruction_listener;
    std::shared_ptr<mf::Shell> const shell;
    std::shared_ptr<mf::SessionAuthorizer> const session_authorizer;
};

static_assert(
    std::is_standard_layout<ClientSessionConstructor>::value,
    "ClientSessionConstructor must be standard layout for wl_container_of to be "
    "defined behaviour.");

void create_client_session(wl_listener* listener, void* data)
{
    auto client = reinterpret_cast<wl_client*>(data);

    ClientSessionConstructor* construction_context;
    construction_context =
        wl_container_of(listener, construction_context, construction_listener);

    pid_t client_pid;
    uid_t client_uid;
    gid_t client_gid;
    wl_client_get_credentials(client, &client_pid, &client_uid, &client_gid);

    auto session_cred = new mf::SessionCredentials{client_pid,
                                                   client_uid, client_gid};
    if (!construction_context->session_authorizer->connection_is_allowed(*session_cred))
    {
        wl_client_destroy(client);
        return;
    }

    auto session = construction_context->shell->open_session(
        client_pid,
        "",
        std::make_shared<WaylandEventSink>([](auto){}));

    auto client_context = new ClientPrivate{session, *construction_context->shell};
    client_context->destroy_listener.notify = &cleanup_private;
    wl_client_add_destroy_listener(client, &client_context->destroy_listener);
}

void cleanup_client_handler(wl_listener* listener, void*)
{
    ClientSessionConstructor* construction_context;
    construction_context = wl_container_of(listener, construction_context, destruction_listener);

    delete construction_context;
}

void setup_new_client_handler(wl_display* display, std::shared_ptr<mf::Shell> const& shell,
                              std::shared_ptr<mf::SessionAuthorizer> const& session_authorizer)
{
    auto context = new ClientSessionConstructor{shell, session_authorizer};
    context->construction_listener.notify = &create_client_session;

    wl_display_add_client_created_listener(display, &context->construction_listener);

    context->destruction_listener.notify = &cleanup_client_handler;
    wl_display_add_destroy_listener(display, &context->destruction_listener);
}

/*
std::shared_ptr<mf::BufferStream> create_buffer_stream(mf::Session& session)
{
    mg::BufferProperties const props{
        geom::Size{geom::Width{0}, geom::Height{0}},
        mir_pixel_format_invalid,
        mg::BufferUsage::undefined
    };

    auto const id = session.create_buffer_stream(props);
    return session.get_buffer_stream(id);
}
*/
MirPixelFormat wl_format_to_mir_format(uint32_t format)
{
    switch (format)
    {
        case WL_SHM_FORMAT_ARGB8888:
            return mir_pixel_format_argb_8888;
        case WL_SHM_FORMAT_XRGB8888:
            return mir_pixel_format_xrgb_8888;
        case WL_SHM_FORMAT_RGBA4444:
            return mir_pixel_format_rgba_4444;
        case WL_SHM_FORMAT_RGBA5551:
            return mir_pixel_format_rgba_5551;
        case WL_SHM_FORMAT_RGB565:
            return mir_pixel_format_rgb_565;
        case WL_SHM_FORMAT_RGB888:
            return mir_pixel_format_rgb_888;
        case WL_SHM_FORMAT_BGR888:
            return mir_pixel_format_bgr_888;
        case WL_SHM_FORMAT_XBGR8888:
            return mir_pixel_format_xbgr_8888;
        case WL_SHM_FORMAT_ABGR8888:
            return mir_pixel_format_abgr_8888;
        default:
            return mir_pixel_format_invalid;
    }
}

wl_shm_buffer* shm_buffer_from_resource_checked(wl_resource* resource)
{
    auto const buffer = wl_shm_buffer_get(resource);
    if (!buffer)
    {
        BOOST_THROW_EXCEPTION((std::logic_error{"Tried to create WlShmBuffer from non-shm resource"}));
    }

    return buffer;
}

template<typename Callable>
auto run_unless(std::shared_ptr<bool> const& condition, Callable&& callable)
{
    return
        [callable = std::move(callable), condition]()
        {
            if (*condition)
                return;

            callable();
        };
}
}

class WlShmBuffer :
    public mg::BufferBasic,
    public mg::NativeBufferBase,
    public mir::renderer::gl::TextureSource,
    public mir::renderer::software::PixelSource
{
public:
    ~WlShmBuffer()
    {
        std::lock_guard<std::mutex> lock{*buffer_mutex};
        if (buffer)
        {
            wl_resource_queue_event(resource, WL_BUFFER_RELEASE);
        }
    }

    static std::shared_ptr<graphics::Buffer> mir_buffer_from_wl_buffer(
        wl_resource* buffer,
        std::function<void()>&& on_consumed)
    {
        std::shared_ptr<WlShmBuffer> mir_buffer;
        DestructionShim* shim;

        if (auto notifier = wl_resource_get_destroy_listener(buffer, &on_buffer_destroyed))
        {
            // We've already constructed a shim for this buffer, update it.
            shim = wl_container_of(notifier, shim, destruction_listener);

            if (!(mir_buffer = shim->associated_buffer.lock()))
            {
                /*
                 * We've seen this wl_buffer before, but all the WlShmBuffers associated with it
                 * have been destroyed.
                 *
                 * Recreate a new WlShmBuffer to track the new compositor lifetime.
                 */
                mir_buffer = std::shared_ptr<WlShmBuffer>{new WlShmBuffer{buffer, std::move(on_consumed)}};
                shim->associated_buffer = mir_buffer;
            }
        }
        else
        {
            mir_buffer = std::shared_ptr<WlShmBuffer>{new WlShmBuffer{buffer, std::move(on_consumed)}};
            shim = new DestructionShim;
            shim->destruction_listener.notify = &on_buffer_destroyed;
            shim->associated_buffer = mir_buffer;

            wl_resource_add_destroy_listener(buffer, &shim->destruction_listener);
        }

        mir_buffer->buffer_mutex = shim->mutex;
        return mir_buffer;
    }

    std::shared_ptr<graphics::NativeBuffer> native_buffer_handle() const override
    {
        return nullptr;
    }

    geometry::Size size() const override
    {
        return size_;
    }

    MirPixelFormat pixel_format() const override
    {
        return format_;
    }

    graphics::NativeBufferBase *native_buffer_base() override
    {
        return this;
    }

    void gl_bind_to_texture() override
    {
        GLenum format, type;

        if (get_gl_pixel_format(
            format_,
            format,
            type))
        {
            /*
             * All existing Mir logic assumes that strides are whole multiples of
             * pixels. And OpenGL defaults to expecting strides are multiples of
             * 4 bytes. These assumptions used to be compatible when we only had
             * 4-byte pixels but now we support 2/3-byte pixels we need to be more
             * careful...
             */
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            read(
               [this, format, type](unsigned char const* pixels)
               {
                   auto const size = this->size();
                   glTexImage2D(GL_TEXTURE_2D, 0, format,
                                size.width.as_int(), size.height.as_int(),
                                0, format, type, pixels);
               });
        }
    }

    void bind() override
    {
        gl_bind_to_texture();
    }

    void secure_for_render() override
    {
    }

    void write(unsigned char const *pixels, size_t size) override
    {
        std::lock_guard<std::mutex> lock{*buffer_mutex};
        wl_shm_buffer_begin_access(buffer);
        auto data = wl_shm_buffer_get_data(buffer);
        ::memcpy(data, pixels, size);
        wl_shm_buffer_end_access(buffer);
    }

    void read(std::function<void(unsigned char const *)> const &do_with_pixels) override
    {
        std::lock_guard<std::mutex> lock{*buffer_mutex};
        if (!buffer)
        {
            mir::log_warning("Attempt to read from WlShmBuffer after the wl_buffer has been destroyed");
            return;
        }

        if (!consumed)
        {
            on_consumed();
            consumed = true;
        }

        do_with_pixels(static_cast<unsigned char const*>(data.get()));
    }

    geometry::Stride stride() const override
    {
        return stride_;
    }

private:
    WlShmBuffer(
        wl_resource* buffer,
        std::function<void()>&& on_consumed)
        : buffer{shm_buffer_from_resource_checked(buffer)},
          resource{buffer},
          size_{wl_shm_buffer_get_width(this->buffer), wl_shm_buffer_get_height(this->buffer)},
          stride_{wl_shm_buffer_get_stride(this->buffer)},
          format_{wl_format_to_mir_format(wl_shm_buffer_get_format(this->buffer))},
          data{std::make_unique<uint8_t[]>(size_.height.as_int() * stride_.as_int())},
          consumed{false},
          on_consumed{std::move(on_consumed)}
    {
        if (stride_.as_int() < size_.width.as_int() * MIR_BYTES_PER_PIXEL(format_))
        {
            wl_resource_post_error(
                resource,
                WL_SHM_ERROR_INVALID_STRIDE,
                "Stride (%u) is less than width × bytes per pixel (%u×%u). "
                "Did you accidentally specify stride in pixels?",
                stride_.as_int(), size_.width.as_int(), MIR_BYTES_PER_PIXEL(format_));

            BOOST_THROW_EXCEPTION((
                std::runtime_error{"Buffer has invalid stride"}));
        }

        wl_shm_buffer_begin_access(this->buffer);
        std::memcpy(data.get(), wl_shm_buffer_get_data(this->buffer), size_.height.as_int() * stride_.as_int());
        wl_shm_buffer_end_access(this->buffer);
    }

    static void on_buffer_destroyed(wl_listener* listener, void*)
    {
        static_assert(
            std::is_standard_layout<DestructionShim>::value,
            "DestructionShim must be Standard Layout for wl_container_of to be defined behaviour");

        DestructionShim* shim;
        shim = wl_container_of(listener, shim, destruction_listener);

        {
            std::lock_guard<std::mutex> lock{*shim->mutex};
            if (auto mir_buffer = shim->associated_buffer.lock())
            {
                mir_buffer->buffer = nullptr;
            }
        }

        delete shim;
    }

    struct DestructionShim
    {
        std::shared_ptr<std::mutex> const mutex = std::make_shared<std::mutex>();
        std::weak_ptr<WlShmBuffer> associated_buffer;
        wl_listener destruction_listener;
    };

    std::shared_ptr<std::mutex> buffer_mutex;

    wl_shm_buffer* buffer;
    wl_resource* const resource;

    geom::Size const size_;
    geom::Stride const stride_;
    MirPixelFormat const format_;

    std::unique_ptr<uint8_t[]> const data;

    bool consumed;
    std::function<void()> on_consumed;
};

class WlSurface : public wayland::Surface
{
public:
    WlSurface(
        wl_client* client,
        wl_resource* parent,
        uint32_t id,
        std::shared_ptr<mir::Executor> const& executor,
        std::shared_ptr<mg::WaylandAllocator> const& allocator)
        : Surface(client, parent, id),
          allocator{allocator},
          executor{executor},
          pending_buffer{nullptr},
          pending_frames{std::make_shared<std::vector<wl_resource*>>()},
          destroyed{std::make_shared<bool>(false)}
    {
        auto session = session_for_client(client);
        mg::BufferProperties const props{
            geom::Size{geom::Width{0}, geom::Height{0}},
            mir_pixel_format_invalid,
            mg::BufferUsage::undefined
        };

        stream_id = session->create_buffer_stream(props);
        stream = session->get_buffer_stream(stream_id);

        // wl_surface is specified to act in mailbox mode
        stream->allow_framedropping(true);
    }

    ~WlSurface()
    {
        *destroyed = true;
        if (auto session = session_for_client(client))
            session->destroy_buffer_stream(stream_id);
    }

    void set_resize_handler(std::function<void(geom::Size)> const& handler)
    {
        resize_handler = handler;
    }

    void set_visibility_handler(std::function<void(bool visible)> const& handler)
    {
        visiblity_handler = handler;
    }

    mf::BufferStreamId stream_id;
    std::shared_ptr<mf::BufferStream> stream;
    mf::SurfaceId surface_id;       // ID of any associated surface

private:
    std::shared_ptr<mg::WaylandAllocator> const allocator;
    std::shared_ptr<mir::Executor> const executor;

    std::function<void(geom::Size)> resize_handler{[](geom::Size){}};
    std::function<void(bool visible)> visiblity_handler{[](bool){}};

    wl_resource* pending_buffer;
    std::shared_ptr<std::vector<wl_resource*>> const pending_frames;
    std::shared_ptr<bool> const destroyed;

    void destroy();
    void attach(std::experimental::optional<wl_resource*> const& buffer, int32_t x, int32_t y);
    void damage(int32_t x, int32_t y, int32_t width, int32_t height);
    void frame(uint32_t callback);
    void set_opaque_region(std::experimental::optional<wl_resource*> const& region);
    void set_input_region(std::experimental::optional<wl_resource*> const& region);
    void commit();
    void damage_buffer(int32_t x, int32_t y, int32_t width, int32_t height);
    void set_buffer_transform(int32_t transform);
    void set_buffer_scale(int32_t scale);
};

void WlSurface::destroy()
{
    wl_resource_destroy(resource);
}

void WlSurface::attach(std::experimental::optional<wl_resource*> const& buffer, int32_t x, int32_t y)
{
    if (x != 0 || y != 0)
    {
        mir::log_warning("Client requested unimplemented non-zero attach offset. Rendering will be incorrect.");
    }

    visiblity_handler(!!buffer);

    pending_buffer = *buffer;
}

void WlSurface::damage(int32_t x, int32_t y, int32_t width, int32_t height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    // This isn't essential, but could enable optimizations
}

void WlSurface::damage_buffer(int32_t x, int32_t y, int32_t width, int32_t height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    // This isn't essential, but could enable optimizations
}

void WlSurface::frame(uint32_t callback)
{
    pending_frames->emplace_back(
        wl_resource_create(client, &wl_callback_interface, 1, callback));
}

void WlSurface::set_opaque_region(const std::experimental::optional<wl_resource*>& region)
{
    (void)region;
}

void WlSurface::set_input_region(const std::experimental::optional<wl_resource*>& region)
{
    (void)region;
}

void WlSurface::commit()
{
    if (pending_buffer)
    {
        auto send_frame_notifications =
            [executor = executor, frames = pending_frames, destroyed = destroyed]()
            {
                executor->spawn(run_unless(
                    destroyed,
                    [frames]()
                    {
                        /*
                         * There is no synchronisation required here -
                         * This is run on the WaylandExecutor, and is guaranteed to run on the
                         * wl_event_loop's thread.
                         *
                         * The only other accessors of WlSurface are also on the wl_event_loop,
                         * so this is guaranteed not to be reentrant.
                         */
                        for (auto frame : *frames)
                        {
                            wl_callback_send_done(frame, 0);
                            wl_resource_destroy(frame);
                        }
                        frames->clear();
                    }));
            };

        std::shared_ptr<mg::Buffer> mir_buffer;

        if (wl_shm_buffer_get(pending_buffer))
        {
            mir_buffer = WlShmBuffer::mir_buffer_from_wl_buffer(
                pending_buffer,
                std::move(send_frame_notifications));
        }
        else
        {
            auto release_buffer = [executor = executor, buffer = pending_buffer, destroyed = destroyed]()
                {
                    executor->spawn(run_unless(
                        destroyed,
                        [buffer](){ wl_resource_queue_event(buffer, WL_BUFFER_RELEASE); }));
                };

            if (allocator &&
                (mir_buffer = allocator->buffer_from_resource(
                    pending_buffer, std::move(send_frame_notifications), std::move(release_buffer))))
            {
            }
            else
            {
                BOOST_THROW_EXCEPTION((std::runtime_error{"Received unhandled buffer type"}));
            }
        }

        /*
         * This is technically incorrect - the resize and submit_buffer *should* be atomic,
         * but are not, so a client in the process of resizing can have buffers rendered at
         * an unexpected size.
         *
         * It should be good enough for now, though.
         *
         * TODO: Provide a mg::Buffer::logical_size() to do this properly.
         */
        stream->resize(mir_buffer->size());
        resize_handler(mir_buffer->size());
        stream->submit_buffer(mir_buffer);

        pending_buffer = nullptr;
    }
}

void WlSurface::set_buffer_transform(int32_t transform)
{
    (void)transform;
    // TODO
}

void WlSurface::set_buffer_scale(int32_t scale)
{
    (void)scale;
    // TODO
}

class WlCompositor : public wayland::Compositor
{
public:
    WlCompositor(
        struct wl_display* display,
        std::shared_ptr<mir::Executor> const& executor,
        std::shared_ptr<mg::WaylandAllocator> const& allocator)
        : Compositor(display, 3),
          allocator{allocator},
          executor{executor}
    {
    }

private:
    std::shared_ptr<mg::WaylandAllocator> const allocator;
    std::shared_ptr<mir::Executor> const executor;

    void create_surface(wl_client* client, wl_resource* resource, uint32_t id) override;
    void create_region(wl_client* client, wl_resource* resource, uint32_t id) override;
};

void WlCompositor::create_surface(wl_client* client, wl_resource* resource, uint32_t id)
{
    new WlSurface{client, resource, id, executor, allocator};
}

class Region : public wayland::Region
{
public:
    Region(wl_client* client, wl_resource* parent, uint32_t id)
        : wayland::Region(client, parent, id)
    {
    }
protected:

    void destroy() override
    {
    }
    void add(int32_t /*x*/, int32_t /*y*/, int32_t /*width*/, int32_t /*height*/) override
    {
    }
    void subtract(int32_t /*x*/, int32_t /*y*/, int32_t /*width*/, int32_t /*height*/) override
    {
    }

};

void WlCompositor::create_region(wl_client* client, wl_resource* resource, uint32_t id)
{
    new Region{client, resource, id};
}

class WlPointer;
class WlTouch;

class WlKeyboard : public wayland::Keyboard
{
public:
    WlKeyboard(
        wl_client* client,
        wl_resource* parent,
        uint32_t id,
        mir::input::Keymap const& initial_keymap,
        std::function<void(WlKeyboard*)> const& on_destroy,
        std::shared_ptr<mir::Executor> const& executor)
        : Keyboard(client, parent, id),
          keymap{nullptr, &xkb_keymap_unref},
          state{nullptr, &xkb_state_unref},
          context{xkb_context_new(XKB_CONTEXT_NO_FLAGS), &xkb_context_unref},
          executor{executor},
          on_destroy{on_destroy},
          destroyed{std::make_shared<bool>(false)}
    {
        // TODO: We should really grab the keymap for the focused surface when
        // we receive focus.

        // TODO: Maintain per-device keymaps, and send the appropriate map before
        // sending an event from a keyboard with a different map.

        /* The wayland::Keyboard constructor has already run, creating the keyboard
         * resource. It is thus safe to send a keymap event to it; the client will receive
         * the keyboard object before this event.
         */
        set_keymap(initial_keymap);
    }

    ~WlKeyboard()
    {
        on_destroy(this);
        *destroyed = true;
    }

    void handle_event(MirInputEvent const* event, wl_resource* /*target*/)
    {
        executor->spawn(run_unless(
            destroyed,
            [
                ev = mcl::Event{mir_event_ref(mir_input_event_get_event(event))},
                this
            ] ()
            {
                auto const serial = wl_display_next_serial(wl_client_get_display(client));
                auto const event = mir_event_get_input_event(ev);
                auto const key_event = mir_input_event_get_keyboard_event(event);
                auto const scancode = mir_keyboard_event_scan_code(key_event);
                /*
                 * HACK! Maintain our own XKB state, so we can serialise it for
                 * wl_keyboard_send_modifiers
                 */

                switch (mir_keyboard_event_action(key_event))
                {
                    case mir_keyboard_action_up:
                        xkb_state_update_key(state.get(), scancode + 8, XKB_KEY_UP);
                        wl_keyboard_send_key(resource,
                            serial,
                            mir_input_event_get_event_time(event) / 1000,
                            mir_keyboard_event_scan_code(key_event),
                            WL_KEYBOARD_KEY_STATE_RELEASED);
                        break;
                    case mir_keyboard_action_down:
                        xkb_state_update_key(state.get(), scancode + 8, XKB_KEY_DOWN);
                        wl_keyboard_send_key(resource,
                            serial,
                            mir_input_event_get_event_time(event) / 1000,
                            mir_keyboard_event_scan_code(key_event),
                            WL_KEYBOARD_KEY_STATE_PRESSED);
                        break;
                    default:
                        break;
                }

                auto new_depressed_mods = xkb_state_serialize_mods(
                    state.get(),
                    XKB_STATE_MODS_DEPRESSED);
                auto new_latched_mods = xkb_state_serialize_mods(
                    state.get(),
                    XKB_STATE_MODS_LATCHED);
                auto new_locked_mods = xkb_state_serialize_mods(
                    state.get(),
                    XKB_STATE_MODS_LOCKED);
                auto new_group = xkb_state_serialize_layout(
                    state.get(),
                    XKB_STATE_LAYOUT_EFFECTIVE);

                if ((new_depressed_mods != mods_depressed) ||
                    (new_latched_mods != mods_latched) ||
                    (new_locked_mods != mods_locked) ||
                    (new_group != group))
                {
                    mods_depressed = new_depressed_mods;
                    mods_latched = new_latched_mods;
                    mods_locked = new_locked_mods;
                    group = new_group;

                    wl_keyboard_send_modifiers(
                        resource,
                        wl_display_get_serial(wl_client_get_display(client)),
                        mods_depressed,
                        mods_latched,
                        mods_locked,
                        group);
                }
            }));
    }

    void handle_event(MirWindowEvent const* event, wl_resource* target)
    {
        if (mir_window_event_get_attribute(event) == mir_window_attrib_focus)
        {
            executor->spawn(run_unless(
                destroyed,
                [
                    target = target,
                    focussed = mir_window_event_get_attribute_value(event),
                    this
                ]()
                {
                    auto const serial = wl_display_next_serial(wl_client_get_display(client));
                    if (focussed)
                    {
                        /*
                         * TODO:
                         *  *) Send the actual modifier state here.
                         *  *) Send the surface's keymap here.
                         */
                        wl_array key_state;
                        wl_array_init(&key_state);
                        wl_keyboard_send_enter(resource, serial, target, &key_state);
                        wl_array_release(&key_state);
                    }
                    else
                    {
                        wl_keyboard_send_leave(resource, serial, target);
                    }
                }));
        }
    }

    void handle_event(MirKeymapEvent const* event, wl_resource* /*target*/)
    {
        char const* buffer;
        size_t length;

        mir_keymap_event_get_keymap_buffer(event, &buffer, &length);

        mir::AnonymousShmFile shm_buffer{length};
        memcpy(shm_buffer.base_ptr(), buffer, length);

        wl_keyboard_send_keymap(
            resource,
            WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
            shm_buffer.fd(),
            length);

        keymap = decltype(keymap)(xkb_keymap_new_from_buffer(
            context.get(),
            buffer,
            length,
            XKB_KEYMAP_FORMAT_TEXT_V1,
            XKB_KEYMAP_COMPILE_NO_FLAGS),
            &xkb_keymap_unref);

        state = decltype(state)(xkb_state_new(keymap.get()), &xkb_state_unref);
    }

    void set_keymap(mir::input::Keymap const& new_keymap)
    {
        xkb_rule_names const names = {
            "evdev",
            new_keymap.model.c_str(),
            new_keymap.layout.c_str(),
            new_keymap.variant.c_str(),
            new_keymap.options.c_str()
        };
        keymap = decltype(keymap){
            xkb_keymap_new_from_names(
                context.get(),
                &names,
                XKB_KEYMAP_COMPILE_NO_FLAGS),
            &xkb_keymap_unref};

        // TODO: We might need to copy across the existing depressed keys?
        state = decltype(state)(xkb_state_new(keymap.get()), &xkb_state_unref);

        auto buffer = xkb_keymap_get_as_string(keymap.get(), XKB_KEYMAP_FORMAT_TEXT_V1);
        auto length = strlen(buffer);

        mir::AnonymousShmFile shm_buffer{length};
        memcpy(shm_buffer.base_ptr(), buffer, length);

        wl_keyboard_send_keymap(
            resource,
            WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
            shm_buffer.fd(),
            length);
    }

private:
    std::unique_ptr<xkb_keymap, decltype(&xkb_keymap_unref)> keymap;
    std::unique_ptr<xkb_state, decltype(&xkb_state_unref)> state;
    std::unique_ptr<xkb_context, decltype(&xkb_context_unref)> const context;

    std::shared_ptr<mir::Executor> const executor;
    std::function<void(WlKeyboard*)> on_destroy;
    std::shared_ptr<bool> const destroyed;

    uint32_t mods_depressed{0};
    uint32_t mods_latched{0};
    uint32_t mods_locked{0};
    uint32_t group{0};

    void release() override;
};

void WlKeyboard::release()
{
    wl_resource_destroy(resource);
}

class WlPointer : public wayland::Pointer
{
public:

    WlPointer(
        wl_client* client,
        wl_resource* parent,
        uint32_t id,
        std::function<void(WlPointer*)> const& on_destroy,
        std::shared_ptr<mir::Executor> const& executor)
        : Pointer(client, parent, id),
          display{wl_client_get_display(client)},
          executor{executor},
          on_destroy{on_destroy},
          destroyed{std::make_shared<bool>(false)}
    {
    }

    ~WlPointer()
    {
        on_destroy(this);
        *destroyed = true;
    }

    void handle_event(MirInputEvent const* event, wl_resource* target)
    {
        executor->spawn(run_unless(
            destroyed,
            [ev = mcl::Event{mir_input_event_get_event(event)}, target, this]()
            {
                auto const serial = wl_display_next_serial(display);
                auto const event = mir_event_get_input_event(ev);
                auto const pointer_event = mir_input_event_get_pointer_event(event);

                switch(mir_pointer_event_action(pointer_event))
                {
                    case mir_pointer_action_button_down:
                    case mir_pointer_action_button_up:
                    {
                        auto const current_set  = mir_pointer_event_buttons(pointer_event);
                        auto const current_time = mir_input_event_get_event_time(event) / 1000;

                        for (auto const& mapping :
                            {
                                std::make_pair(mir_pointer_button_primary, BTN_LEFT),
                                std::make_pair(mir_pointer_button_secondary, BTN_RIGHT),
                                std::make_pair(mir_pointer_button_tertiary, BTN_MIDDLE),
                                std::make_pair(mir_pointer_button_back, BTN_BACK),
                                std::make_pair(mir_pointer_button_forward, BTN_FORWARD),
                                std::make_pair(mir_pointer_button_side, BTN_SIDE),
                                std::make_pair(mir_pointer_button_task, BTN_TASK),
                                std::make_pair(mir_pointer_button_extra, BTN_EXTRA)
                            })
                        {
                            if (mapping.first & (current_set ^ last_set))
                            {
                                auto const action = (mapping.first & current_set) ?
                                                    WL_POINTER_BUTTON_STATE_PRESSED :
                                                    WL_POINTER_BUTTON_STATE_RELEASED;

                                wl_pointer_send_button(resource, serial, current_time, mapping.second, action);
                            }
                        }

                        last_set = current_set;
                        break;
                    }
                    case mir_pointer_action_enter:
                    {
                        wl_pointer_send_enter(
                            resource,
                            serial,
                            target,
                            wl_fixed_from_double(mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x)),
                            wl_fixed_from_double(mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y)));
                        break;
                    }
                    case mir_pointer_action_leave:
                    {
                        wl_pointer_send_leave(
                            resource,
                            serial,
                            target);
                        break;
                    }
                    case mir_pointer_action_motion:
                    {
                        auto x = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x);
                        auto y = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y);
                        auto vscroll = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_vscroll);
                        auto hscroll = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_hscroll);

                        if ((x != last_x) || (y != last_y))
                        {
                            wl_pointer_send_motion(
                                resource,
                                mir_input_event_get_event_time(event) / 1000,
                                wl_fixed_from_double(x),
                                wl_fixed_from_double(y));

                            last_x = x;
                            last_y = y;
                        }
                        if (vscroll != last_vscroll)
                        {
                            wl_pointer_send_axis(
                                resource,
                                mir_input_event_get_event_time(event) / 1000,
                                WL_POINTER_AXIS_VERTICAL_SCROLL,
                                wl_fixed_from_double(vscroll));
                            last_vscroll = vscroll;
                        }
                        if (hscroll != last_hscroll)
                        {
                            wl_pointer_send_axis(
                                resource,
                                mir_input_event_get_event_time(event) / 1000,
                                WL_POINTER_AXIS_HORIZONTAL_SCROLL,
                                wl_fixed_from_double(hscroll));
                            last_hscroll = hscroll;
                        }
                        break;
                    }
                    case mir_pointer_actions:
                        break;
                }
            }));
    }

    // Pointer interface
private:
    wl_display* const display;
    std::shared_ptr<mir::Executor> const executor;

    std::function<void(WlPointer*)> on_destroy;
    std::shared_ptr<bool> const destroyed;

    MirPointerButtons last_set{0};
    float last_x, last_y, last_vscroll, last_hscroll;

    void set_cursor(uint32_t serial, std::experimental::optional<wl_resource*> const& surface, int32_t hotspot_x, int32_t hotspot_y) override;
    void release() override;
};

void WlPointer::set_cursor(uint32_t serial, std::experimental::optional<wl_resource*> const& surface, int32_t hotspot_x, int32_t hotspot_y)
{
    (void)serial;
    (void)surface;
    (void)hotspot_x;
    (void)hotspot_y;
}

void WlPointer::release()
{
    wl_resource_destroy(resource);
}

class WlTouch : public wayland::Touch
{
public:
    WlTouch(
        wl_client* client,
        wl_resource* parent,
        uint32_t id,
        std::function<void(WlTouch*)> const& on_destroy,
        std::shared_ptr<mir::Executor> const& executor)
        : Touch(client, parent, id),
          executor{executor},
          on_destroy{on_destroy},
          destroyed{std::make_shared<bool>(false)}
    {
    }

    ~WlTouch()
    {
        on_destroy(this);
        *destroyed = true;
    }

    void handle_event(MirInputEvent const* event, wl_resource* target)
    {
        executor->spawn(run_unless(
            destroyed,
            [ev = mcl::Event{mir_input_event_get_event(event)}, target = target, this]()
            {
                auto const input_ev = mir_event_get_input_event(ev);
                auto const touch_ev = mir_input_event_get_touch_event(input_ev);

                for (auto i = 0u; i < mir_touch_event_point_count(touch_ev); ++i)
                {
                    auto const touch_id = mir_touch_event_id(touch_ev, i);
                    auto const action = mir_touch_event_action(touch_ev, i);
                    auto const x = mir_touch_event_axis_value(
                        touch_ev,
                        i,
                        mir_touch_axis_x);
                    auto const y = mir_touch_event_axis_value(
                        touch_ev,
                        i,
                        mir_touch_axis_y);

                    switch (action)
                    {
                    case mir_touch_action_down:
                        wl_touch_send_down(
                            resource,
                            wl_display_get_serial(wl_client_get_display(client)),
                            mir_input_event_get_event_time(input_ev) / 1000,
                            target,
                            touch_id,
                            wl_fixed_from_double(x),
                            wl_fixed_from_double(y));
                        break;
                    case mir_touch_action_up:
                        wl_touch_send_up(
                            resource,
                            wl_display_get_serial(wl_client_get_display(client)),
                            mir_input_event_get_event_time(input_ev) / 1000,
                            touch_id);
                        break;
                    case mir_touch_action_change:
                        wl_touch_send_motion(
                            resource,
                            mir_input_event_get_event_time(input_ev) / 1000,
                            touch_id,
                            wl_fixed_from_double(x),
                            wl_fixed_from_double(y));
                        break;
                    case mir_touch_actions:
                        /*
                         * We should never receive an event with this action set;
                         * the only way would be if a *new* action has been added
                         * to the enum, and this hasn't been updated.
                         *
                         * There's nothing to do here, but don't use default: so
                         * that the compiler will warn if a new enum value is added.
                         */
                        break;
                    }
                }

                if (mir_touch_event_point_count(touch_ev) > 0)
                {
                    /*
                     * This is mostly paranoia; I assume we won't actually be called
                     * with an empty touch event.
                     *
                     * Regardless, the Wayland protocol requires that there be at least
                     * one event sent before we send the ending frame, so make that explicit.
                     */
                    wl_touch_send_frame(resource);
                }
            }
            ));
    }

    // Touch interface
private:
    std::shared_ptr<mir::Executor> const executor;
    std::function<void(WlTouch*)> on_destroy;
    std::shared_ptr<bool> const destroyed;

    void release() override;
};

void WlTouch::release()
{
    wl_resource_destroy(resource);
}

template<class InputInterface>
class InputCtx
{
public:
    InputCtx() = default;

    InputCtx(InputCtx&&) = delete;
    InputCtx(InputCtx const&) = delete;
    InputCtx& operator=(InputCtx const&) = delete;

    void register_listener(InputInterface* listener)
    {
        std::lock_guard<std::mutex> lock{mutex};
        listeners.push_back(listener);
    }

    void unregister_listener(InputInterface const* listener)
    {
        std::lock_guard<std::mutex> lock{mutex};
        listeners.erase(
            std::remove(
                listeners.begin(),
                listeners.end(),
                listener),
            listeners.end());
    }

    void handle_event(MirInputEvent const* event, wl_resource* target) const
    {
        std::lock_guard<std::mutex> lock{mutex};
        for (auto listener : listeners)
        {
            listener->handle_event(event, target);
        }
    }

    void handle_event(MirWindowEvent const* event, wl_resource* target) const
    {
        std::lock_guard<std::mutex> lock{mutex};
        for (auto& listener : listeners)
        {
            listener->handle_event(event, target);
        }
    }

    void handle_event(MirKeymapEvent const* event, wl_resource* target) const
    {
        std::lock_guard<std::mutex> lock{mutex};
        for (auto& listener : listeners)
        {
            listener->handle_event(event, target);
        }
    }

private:
    std::mutex mutable mutex;
    std::vector<InputInterface*> listeners;
};

class WlSeat
{
public:
    WlSeat(
        wl_display* display,
        std::shared_ptr<mi::InputDeviceHub> const& input_hub,
        std::shared_ptr<mir::Executor> const& executor)
        : config_observer{
              std::make_shared<ConfigObserver>(
                  keymap,
                  [this](mir::input::Keymap const& new_keymap)
                  {
                      keymap = new_keymap;

                  })},
          input_hub{input_hub},
          executor{executor},
          global{wl_global_create(
              display,
              &wl_seat_interface,
              5,
              this,
              &WlSeat::bind)}
    {
        if (!global)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("Failed to export wl_seat interface"));
        }
        input_hub->add_observer(config_observer);
    }

    ~WlSeat()
    {
        input_hub->remove_observer(config_observer);
        wl_global_destroy(global);
    }

    InputCtx<WlPointer> const& acquire_pointer_reference(wl_client* client) const;
    InputCtx<WlKeyboard> const& acquire_keyboard_reference(wl_client* client) const;
    InputCtx<WlTouch> const& acquire_touch_reference(wl_client* client) const;

    void spawn(std::function<void()>&& work)
    {
        executor->spawn(std::move(work));
    }

private:
    class ConfigObserver :  public mi::InputDeviceObserver
    {
    public:
        ConfigObserver(
            mir::input::Keymap const& keymap,
            std::function<void(mir::input::Keymap const&)> const& on_keymap_commit)
            : current_keymap{keymap},
              on_keymap_commit{on_keymap_commit}
        {
        }

        void device_added(std::shared_ptr<input::Device> const& device) override;
        void device_changed(std::shared_ptr<input::Device> const& device) override;
        void device_removed(std::shared_ptr<input::Device> const& device) override;
        void changes_complete() override;

    private:
        mir::input::Keymap const& current_keymap;
        mir::input::Keymap pending_keymap;
        std::function<void(mir::input::Keymap const&)> const on_keymap_commit;
    };

    mir::input::Keymap keymap;
    std::shared_ptr<ConfigObserver> const config_observer;

    std::unordered_map<wl_client*, InputCtx<WlPointer>> mutable pointer;
    std::unordered_map<wl_client*, InputCtx<WlKeyboard>> mutable keyboard;
    std::unordered_map<wl_client*, InputCtx<WlTouch>> mutable touch;

    std::shared_ptr<mi::InputDeviceHub> const input_hub;


    std::shared_ptr<mir::Executor> const executor;

    static void bind(struct wl_client* client, void* data, uint32_t version, uint32_t id)
    {
        auto me = reinterpret_cast<WlSeat*>(data);
        auto resource = wl_resource_create(client, &wl_seat_interface,
            std::min(version, 6u), id);
        if (resource == nullptr)
        {
            wl_client_post_no_memory(client);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, me, nullptr);

        /*
         * TODO: Read the actual capabilities. Do we have a keyboard? Mouse? Touch?
         */
        if (version >= WL_SEAT_CAPABILITIES_SINCE_VERSION)
        {
            wl_seat_send_capabilities(
                resource,
                WL_SEAT_CAPABILITY_POINTER |
                WL_SEAT_CAPABILITY_KEYBOARD |
                WL_SEAT_CAPABILITY_TOUCH);
        }
        if (version >= WL_SEAT_NAME_SINCE_VERSION)
        {
            wl_seat_send_name(
                resource,
                "seat0");
        }

        wl_resource_set_user_data(resource, me);
    }

    static void get_pointer(wl_client* client, wl_resource* resource, uint32_t id)
    {
        auto me = reinterpret_cast<WlSeat*>(wl_resource_get_user_data(resource));
        auto& input_ctx = me->pointer[client];
        input_ctx.register_listener(
            new WlPointer{
                client,
                resource,
                id,
                [&input_ctx](WlPointer* listener)
                {
                    input_ctx.unregister_listener(listener);
                },
                me->executor});
    }
    static void get_keyboard(wl_client* client, wl_resource* resource, uint32_t id)
    {
        auto me = reinterpret_cast<WlSeat*>(wl_resource_get_user_data(resource));
        auto& input_ctx = me->keyboard[client];

        input_ctx.register_listener(
            new WlKeyboard{
                client,
                resource,
                id,
                me->keymap,
                [&input_ctx](WlKeyboard* listener)
                {
                    input_ctx.unregister_listener(listener);
                },
                me->executor});
    }
    static void get_touch(wl_client* client, wl_resource* resource, uint32_t id)
    {
        auto me = reinterpret_cast<WlSeat*>(wl_resource_get_user_data(resource));
        auto& input_ctx = me->touch[client];

        input_ctx.register_listener(
            new WlTouch{
                client,
                resource,
                id,
                [&input_ctx](WlTouch* listener)
                {
                    input_ctx.unregister_listener(listener);
                },
                me->executor});
    }
    static void release(struct wl_client* /*client*/, struct wl_resource* us)
    {
        wl_resource_destroy(us);
    }

    wl_global* const global;
    static struct wl_seat_interface const vtable;
};

struct wl_seat_interface const WlSeat::vtable = {
    WlSeat::get_pointer,
    WlSeat::get_keyboard,
    WlSeat::get_touch,
    WlSeat::release
};

InputCtx<WlKeyboard> const& WlSeat::acquire_keyboard_reference(wl_client* client) const
{
    return keyboard[client];
}

InputCtx<WlPointer> const& WlSeat::acquire_pointer_reference(wl_client* client) const
{
    return pointer[client];
}

InputCtx<WlTouch> const& WlSeat::acquire_touch_reference(wl_client* client) const
{
    return touch[client];
}

void WlSeat::ConfigObserver::device_added(std::shared_ptr<input::Device> const& device)
{
    if (auto keyboard_config = device->keyboard_configuration())
    {
        if (current_keymap != keyboard_config.value().device_keymap())
        {
            pending_keymap = keyboard_config.value().device_keymap();
        }
    }
}

void WlSeat::ConfigObserver::device_changed(std::shared_ptr<input::Device> const& device)
{
    if (auto keyboard_config = device->keyboard_configuration())
    {
        if (current_keymap != keyboard_config.value().device_keymap())
        {
            pending_keymap = keyboard_config.value().device_keymap();
        }
    }
}

void WlSeat::ConfigObserver::device_removed(std::shared_ptr<input::Device> const& /*device*/)
{
}

void WlSeat::ConfigObserver::changes_complete()
{
    on_keymap_commit(pending_keymap);
}

void WaylandEventSink::send_buffer(BufferStreamId /*id*/, graphics::Buffer& /*buffer*/, graphics::BufferIpcMsgType)
{
}

void WaylandEventSink::handle_event(MirEvent const& e)
{
    switch(mir_event_get_type(&e))
    {
    default:
        // Do nothing
        break;
    }
}

void WaylandEventSink::handle_lifecycle_event(MirLifecycleState state)
{
    lifecycle_handler(state);
}

void WaylandEventSink::handle_display_config_change(graphics::DisplayConfiguration const& /*config*/)
{
}

void WaylandEventSink::send_ping(int32_t)
{
}

class BasicSurfaceEventSink : public mf::EventSink
{
public:
    BasicSurfaceEventSink(WlSeat* seat, wl_client* client, wl_resource* target, wl_resource* event_sink)
        : seat{seat},
          client{client},
          target{target},
          event_sink{event_sink},
          window_size{geometry::Size{0,0}}
    {
    }

    void handle_event(MirEvent const& e) override;
    void handle_lifecycle_event(MirLifecycleState) override {}
    void handle_display_config_change(graphics::DisplayConfiguration const&) override {}
    void send_ping(int32_t) override {}
    void handle_input_config_change(MirInputConfig const&) override {}
    void handle_error(ClientVisibleError const&) override {}

    void send_buffer(frontend::BufferStreamId, graphics::Buffer&, graphics::BufferIpcMsgType) override {}
    void add_buffer(graphics::Buffer&) override {}
    void error_buffer(geometry::Size, MirPixelFormat, std::string const& ) override {}
    void update_buffer(graphics::Buffer&) override {}

    void latest_resize(geometry::Size window_size)
    {
        this->window_size = window_size;
    }

    virtual void handle_resize_event(MirResizeEvent const* event) = 0;

protected:
    WlSeat* const seat;
    wl_client* const client;
    wl_resource* const target;
    wl_resource* const event_sink;
    std::atomic<geometry::Size> window_size;
};

void BasicSurfaceEventSink::handle_event(MirEvent const& event)
{
    switch (mir_event_get_type(&event))
    {
    case mir_event_type_resize:
    {
        handle_resize_event(mir_event_get_resize_event(&event));
        break;
    }
    case mir_event_type_input:
    {
        auto input_event = mir_event_get_input_event(&event);
        switch (mir_input_event_get_type(input_event))
        {
        case mir_input_event_type_key:
            seat->acquire_keyboard_reference(client).handle_event(input_event, target);
            break;
        case mir_input_event_type_pointer:
            seat->acquire_pointer_reference(client).handle_event(input_event, target);
            break;
        case mir_input_event_type_touch:
            seat->acquire_touch_reference(client).handle_event(input_event, target);
            break;
        default:
            break;
        }
        break;
    }
    case mir_event_type_keymap:
    {
        auto const map_ev = mir_event_get_keymap_event(&event);

        seat->acquire_keyboard_reference(client).handle_event(map_ev, target);
        break;
    }
    case mir_event_type_window:
    {
        auto const wev = mir_event_get_window_event(&event);

        seat->acquire_keyboard_reference(client).handle_event(wev, target);
    }
    default:
        break;
    }
}

class SurfaceEventSink : public BasicSurfaceEventSink
{
public:
    using BasicSurfaceEventSink::BasicSurfaceEventSink;

    void handle_resize_event(MirResizeEvent const* event) override;
};

void SurfaceEventSink::handle_resize_event(MirResizeEvent const* event)
{
    geom::Size new_size{mir_resize_event_get_width(event), mir_resize_event_get_height(event)};
    if (window_size != new_size)
    {
        seat->spawn([event_sink= event_sink,
                        width = mir_resize_event_get_width(event),
                        height = mir_resize_event_get_height(event)]()
                        {
                            wl_shell_surface_send_configure(event_sink, WL_SHELL_SURFACE_RESIZE_NONE, width, height);
                        });
    }
}

class Output
{
public:
    Output(
        wl_display* display,
        mg::DisplayConfigurationOutput const& initial_configuration)
        : output{make_output(display)},
          current_config{initial_configuration}
    {
    }

    ~Output()
    {
        wl_global_destroy(output);
    }

    void handle_configuration_changed(mg::DisplayConfigurationOutput const& /*config*/)
    {

    }

private:
    static void send_initial_config(
        wl_resource* client_resource,
        mg::DisplayConfigurationOutput const& config)
    {
        wl_output_send_geometry(
            client_resource,
            config.top_left.x.as_int(),
            config.top_left.y.as_int(),
            config.physical_size_mm.width.as_int(),
            config.physical_size_mm.height.as_int(),
            WL_OUTPUT_SUBPIXEL_UNKNOWN,
            "Fake manufacturer",
            "Fake model",
            WL_OUTPUT_TRANSFORM_NORMAL);
        for (size_t i = 0; i < config.modes.size(); ++i)
        {
            auto const& mode = config.modes[i];
            wl_output_send_mode(
                client_resource,
                ((i == config.preferred_mode_index ? WL_OUTPUT_MODE_PREFERRED : 0) |
                 (i == config.current_mode_index ? WL_OUTPUT_MODE_CURRENT : 0)),
                mode.size.width.as_int(),
                mode.size.height.as_int(),
                mode.vrefresh_hz * 1000);
        }
        wl_output_send_scale(client_resource, 1);
        wl_output_send_done(client_resource);
    }

    wl_global* make_output(wl_display* display)
    {
        return wl_global_create(
            display,
            &wl_output_interface,
            2,
            this, &on_bind);
    }

    static void on_bind(wl_client* client, void* data, uint32_t version, uint32_t id)
    {
        auto output = reinterpret_cast<Output*>(data);
        auto resource = wl_resource_create(client, &wl_output_interface,
            std::min(version, 2u), id);
        if (resource == NULL) {
            wl_client_post_no_memory(client);
            return;
        }

        output->resource_map[client].push_back(resource);
        wl_resource_set_destructor(resource, &resource_destructor);
        wl_resource_set_user_data(resource, &(output->resource_map));

        send_initial_config(resource, output->current_config);
    }

    static void resource_destructor(wl_resource* resource)
    {
        auto& map = *reinterpret_cast<decltype(resource_map)*>(
            wl_resource_get_user_data(resource));

        auto& client_resource_list = map[wl_resource_get_client(resource)];
        std::remove_if(
            client_resource_list.begin(),
            client_resource_list.end(),
            [resource](auto candidate) { return candidate == resource; });
    }

private:
    wl_global* const output;
    mg::DisplayConfigurationOutput current_config;
    std::unordered_map<wl_client*, std::vector<wl_resource*>> resource_map;
};

class OutputManager
{
public:
    OutputManager(
        wl_display* display,
        mf::DisplayChanger& display_config)
        : display{display}
    {
        // TODO: Also register display configuration listeners
        display_config.base_configuration()->for_each_output(std::bind(&OutputManager::create_output, this, std::placeholders::_1));
    }

private:
    void create_output(mg::DisplayConfigurationOutput const& initial_config)
    {
        if (initial_config.used)
        {
            outputs.emplace(
                initial_config.id,
                std::make_unique<Output>(
                    display,
                    initial_config));
        }
    }

    void handle_configuration_change(mg::DisplayConfiguration const& config)
    {
        config.for_each_output([this](mg::DisplayConfigurationOutput const& output_config)
            {
                auto output_iter = outputs.find(output_config.id);
                if (output_iter != outputs.end())
                {
                    if (output_config.used)
                    {
                        output_iter->second->handle_configuration_changed(output_config);
                    }
                    else
                    {
                        outputs.erase(output_iter);
                    }
                }
                else if (output_config.used)
                {
                    outputs[output_config.id] = std::make_unique<Output>(display, output_config);
                }
            });
    }

    wl_display* const display;
    std::unordered_map<mg::DisplayConfigurationOutputId, std::unique_ptr<Output>> outputs;
};

class WlShellSurface : public wayland::ShellSurface
{
public:
    WlShellSurface(
        wl_client* client,
        wl_resource* parent,
        uint32_t id,
        wl_resource* surface,
        std::shared_ptr<mf::Shell> const& shell,
        WlSeat& seat)
        : ShellSurface(client, parent, id),
          destroyed{std::make_shared<bool>(false)},
          shell{shell}
    {
        auto* const mir_surface = get_wlsurface(surface);

        auto const sink = std::make_shared<SurfaceEventSink>(&seat, client, surface, resource);

        mir_surface->set_resize_handler([mir_surface, sink, &seat, this](geom::Size initial_size)
                                            { resize_handler(mir_surface, sink, seat, initial_size); });
    }

    ~WlShellSurface() override
    {
        *destroyed = true;
        if (surface_id.as_value())
        {
            if (auto session = session_for_client(client))
            {
                shell->destroy_surface(session, surface_id);
            }
        }
    }
protected:
    void pong(uint32_t /*serial*/) override
    {
    }

    void move(struct wl_resource* /*seat*/, uint32_t /*serial*/) override
    {
    }

    void resize(struct wl_resource* /*seat*/, uint32_t /*serial*/, uint32_t /*edges*/) override
    {
    }

    void set_toplevel() override
    {
    }

    void set_transient(
        struct wl_resource* parent,
        int32_t x,
        int32_t y,
        uint32_t /*flags*/) override
    {
        auto const session = session_for_client(client);
        auto& parent_surface = *get_wlsurface(parent);

        if (surface_id.as_value())
        {
            shell::SurfaceSpecification new_spec;
            new_spec.parent_id = parent_surface.surface_id;
            new_spec.aux_rect = geom::Rectangle{{x, y}, {}};
            new_spec.surface_placement_gravity = mir_placement_gravity_northwest;
            new_spec.aux_rect_placement_gravity = mir_placement_gravity_southeast;
            new_spec.placement_hints = mir_placement_hints_slide_x;
            new_spec.aux_rect_placement_offset_x = 0;
            new_spec.aux_rect_placement_offset_y = 0;
            shell->modify_surface(session, surface_id, new_spec);
        }
        else
        {
            params.type = mir_window_type_gloss;
            params.parent_id = parent_surface.surface_id;
            params.aux_rect = geom::Rectangle{{x, y}, {}};
            params.surface_placement_gravity = mir_placement_gravity_northwest;
            params.aux_rect_placement_gravity = mir_placement_gravity_southeast;
            params.placement_hints = mir_placement_hints_slide_x;
            params.aux_rect_placement_offset_x = 0;
            params.aux_rect_placement_offset_y = 0;
        }
    }

    void set_fullscreen(
        uint32_t /*method*/,
        uint32_t /*framerate*/,
        std::experimental::optional<struct wl_resource*> const& output) override
    {
        if (surface_id.as_value())
        {
            mir::shell::SurfaceSpecification mods;
            mods.state = mir_window_state_fullscreen;
            if (output)
            {
                // TODO{alan_g} mods.output_id = DisplayConfigurationOutputId_from(output)
            }

            auto const session = session_for_client(client);
            shell->modify_surface(session, surface_id, mods);
        }
        else
        {
            params.state = mir_window_state_fullscreen;
            if (output)
            {
                // TODO{alan_g} params.output_id = DisplayConfigurationOutputId_from(output)
            }
        }
    }

    void set_popup(
        struct wl_resource* /*seat*/,
        uint32_t /*serial*/,
        struct wl_resource* parent,
        int32_t x,
        int32_t y,
        uint32_t /*flags*/) override
    {
        auto const session = session_for_client(client);
        auto& parent_surface = *get_wlsurface(parent);

        if (surface_id.as_value())
        {
            shell::SurfaceSpecification new_spec;
            new_spec.parent_id = parent_surface.surface_id;
            new_spec.aux_rect = geom::Rectangle{{x, y}, {}};
            new_spec.surface_placement_gravity = mir_placement_gravity_northwest;
            new_spec.aux_rect_placement_gravity = mir_placement_gravity_southeast;
            new_spec.placement_hints = mir_placement_hints_slide_x;
            new_spec.aux_rect_placement_offset_x = 0;
            new_spec.aux_rect_placement_offset_y = 0;
            shell->modify_surface(session, surface_id, new_spec);
        }
        else
        {
            params.parent_id = parent_surface.surface_id;
            params.aux_rect = geom::Rectangle{{x, y}, {}};
            params.surface_placement_gravity = mir_placement_gravity_northwest;
            params.aux_rect_placement_gravity = mir_placement_gravity_southeast;
            params.placement_hints = mir_placement_hints_slide_x;
            params.aux_rect_placement_offset_x = 0;
            params.aux_rect_placement_offset_y = 0;
        }
    }

    void set_maximized(std::experimental::optional<struct wl_resource*> const& output) override
    {
        if (surface_id.as_value())
        {
            mir::shell::SurfaceSpecification mods;
            mods.state = mir_window_state_maximized;
            if (output)
            {
                // TODO{alan_g} mods.output_id = DisplayConfigurationOutputId_from(output)
            }
            auto const session = session_for_client(client);
            shell->modify_surface(session, surface_id, mods);
        }
        else
        {
            params.state = mir_window_state_maximized;
            if (output)
            {
                // TODO{alan_g} params.output_id = DisplayConfigurationOutputId_from(output)
            }
        }
    }

    void set_title(std::string const& title) override
    {
        if (surface_id.as_value())
        {
            shell::SurfaceSpecification new_spec;
            new_spec.name = title;
            auto const session = session_for_client(client);
            shell->modify_surface(session, surface_id, new_spec);
        }
        else
        {
            params.name = title;
        }
    }

    void set_class(std::string const& /*class_*/) override
    {
    }

private:
    void resize_handler(
        WlSurface* mir_surface,
        std::shared_ptr<SurfaceEventSink> const& sink,
        WlSeat& seat,
        geom::Size initial_size)
    {
        auto const session = session_for_client(client);

        if (surface_id.as_value())
        {
            auto const surface = get_surface_for_id(session, surface_id);
            if (surface->size() == initial_size)
                return;
            sink->latest_resize(initial_size);
            shell::SurfaceSpecification new_size_spec;
            new_size_spec.width = initial_size.width;
            new_size_spec.height = initial_size.height;
            shell->modify_surface(session, surface_id, new_size_spec);
            return;
        }

        params.size = initial_size;
        params.content_id = mir_surface->stream_id;
        surface_id = shell->create_surface(session, params, sink);
        mir_surface->surface_id = surface_id;

        {
            // The shell isn't guaranteed to respect the requested size
            auto const window = session->get_surface(surface_id);
            auto const size = window->client_size();

            if (size != initial_size)
            {
                sink->latest_resize(size);
                seat.spawn(
                    run_unless(
                        destroyed,
                        [resource = resource, height = size.height.as_int(), width = size.width.as_int()]()
                            {
                                wl_shell_surface_send_configure(
                                    resource, WL_SHELL_SURFACE_RESIZE_NONE, width, height);
                            }));
            }
        }

        mir_surface->set_visibility_handler(
            [shell=shell, session, id = surface_id](bool visible)
                {
                    if (get_surface_for_id(session, id)->visible() == visible)
                        return;
                    shell::SurfaceSpecification hide_spec;
                    hide_spec.state = visible ? mir_window_state_restored : mir_window_state_hidden;
                    shell->modify_surface(session, id, hide_spec);
                });
    }

    WlSurface* get_wlsurface(wl_resource* surface) const
    {
        auto* tmp = wl_resource_get_user_data(surface);
        return static_cast<WlSurface*>(static_cast<wayland::Surface*>(tmp));
    }

    std::shared_ptr<bool> const destroyed;
    std::shared_ptr<mf::Shell> const shell;
    mf::SurfaceId surface_id;

    ms::SurfaceCreationParameters params = ms::SurfaceCreationParameters().of_type(mir_window_type_freestyle);
};

class WlShell : public wayland::Shell
{
public:
    WlShell(
        wl_display* display,
        std::shared_ptr<mf::Shell> const& shell,
        WlSeat& seat)
        : Shell(display, 1),
          shell{shell},
          seat{seat}
    {
    }

    void get_shell_surface(
        wl_client* client,
        wl_resource* resource,
        uint32_t id,
        wl_resource* surface) override
    {
        new WlShellSurface(client, resource, id, surface, shell, seat);
    }
private:
    std::shared_ptr<mf::Shell> const shell;
    WlSeat& seat;
};

class XdgToplevelV6 : public wayland::XdgToplevelV6
{
public:
    XdgToplevelV6(wl_client* client,
        wl_resource* parent,
        uint32_t id)
        : wayland::XdgToplevelV6(client, parent, id),
          destroyed{std::make_shared<bool>(false)}
    {
        // TODO: it appears an executer is used to run send the events at a later time elsewhere in the code. is this needed here?
        send_configure();
    }

    ~XdgToplevelV6()
    {
        *destroyed = true;
    }

    void destroy() override
    {
    }

    void set_parent(std::experimental::optional<struct wl_resource*> const& /*parent*/) override
    {
    }

    void set_title(std::string const& /*title*/) override
    {
    }

    void set_app_id(std::string const& /*app_id*/) override
    {
    }

    void show_window_menu(struct wl_resource* /*seat*/, uint32_t /*serial*/, int32_t /*x*/, int32_t /*y*/) override
    {
    }

    void move(struct wl_resource* /*seat*/, uint32_t /*serial*/) override
    {
    }

    void resize(struct wl_resource* /*seat*/, uint32_t /*serial*/, uint32_t /*edges*/) override
    {
    }

    void set_max_size(int32_t /*width*/, int32_t /*height*/) override
    {
    }

    void set_min_size(int32_t /*width*/, int32_t /*height*/) override
    {
    }

    void set_maximized() override
    {
    }

    void unset_maximized() override
    {
    }

    void set_fullscreen(std::experimental::optional<struct wl_resource*> const& /*output*/) override
    {
    }

    void unset_fullscreen() override
    {
    }

    void set_minimized() override
    {
    }

private:
    void send_configure()
    {
        wl_array config_array;
        wl_array_init(&config_array);
        auto item_ptr = static_cast<zxdg_toplevel_v6_state *>(wl_array_add(&config_array, sizeof(zxdg_toplevel_v6_state)));
        *item_ptr = ZXDG_TOPLEVEL_V6_STATE_ACTIVATED;
        zxdg_toplevel_v6_send_configure(resource, 0, 0, &config_array);
        wl_array_release(&config_array);
    }

    std::shared_ptr<bool> const destroyed;
};

class XdgSurfaceV6 : public wayland::XdgSurfaceV6
{
public:
    XdgSurfaceV6(
        wl_display* display,
        wl_client* client,
        wl_resource* parent,
        uint32_t id,
        wl_resource* surface,
        std::shared_ptr<mf::Shell> const& shell,
        WlSeat& seat)
        : wayland::XdgSurfaceV6(client, parent, id),
          destroyed{std::make_shared<bool>(false)},
          display{display},
          shell{shell}
    {
        auto* tmp = wl_resource_get_user_data(surface);
        auto& mir_surface = *static_cast<WlSurface*>(tmp);

        auto const session = session_for_client(client);

        auto params = ms::SurfaceCreationParameters()
            .of_type(mir_window_type_freestyle)
            .of_size(geom::Size{640, 480})
            .with_buffer_stream(mir_surface.stream_id);

        auto const sink = std::make_shared<SurfaceEventSink>(&seat, client, surface, resource);
        surface_id = shell->create_surface(session, params, sink);

        {
            // The shell isn't guaranteed to respect the requested size
            auto const window = session->get_surface(surface_id);
            auto const size = window->client_size();
            sink->latest_resize(size);
        }

        mir_surface.set_resize_handler(
            [shell, session, id = surface_id, sink](geom::Size new_size)
            {
                sink->latest_resize(new_size);
                shell::SurfaceSpecification new_size_spec;
                new_size_spec.width = new_size.width;
                new_size_spec.height = new_size.height;
                shell->modify_surface(session, id, new_size_spec);
            });

        mir_surface.set_hide_handler(
            [shell, session, id = surface_id]()
            {
                shell::SurfaceSpecification hide_spec;
                hide_spec.state = mir_window_state_hidden;
                shell->modify_surface(session, id, hide_spec);
            });
    }

    ~XdgSurfaceV6()
    {
        *destroyed = true;
        if (auto session = session_for_client(client))
        {
            shell->destroy_surface(session, surface_id);
        }
    }

    void destroy() override
    {
        // TODO: do we need to destroy something?
    }

    void get_toplevel(uint32_t id) override
    {
        new XdgToplevelV6(client, resource, id);
        zxdg_surface_v6_send_configure(resource, wl_display_next_serial(display));
    }

    void get_popup(uint32_t /*id*/, struct wl_resource* /*parent*/, struct wl_resource* /*positioner*/) override
    {
        // TODO
    }

    void set_window_geometry(int32_t /*x*/, int32_t /*y*/, int32_t /*width*/, int32_t /*height*/) override
    {
        // TODO
    }

    void ack_configure(uint32_t /*serial*/) override
    {
        // used to know when a client has responded to configure event, not needed for now
    }

private:
    std::shared_ptr<bool> const destroyed;
    wl_display* display;
    std::shared_ptr<mf::Shell> const shell;
    mf::SurfaceId surface_id;
};

class XdgShellV6 : public wayland::XdgShellV6
{
public:
    XdgShellV6(
        wl_display* display,
        std::shared_ptr<mf::Shell> const& shell,
        WlSeat& seat)
        : wayland::XdgShellV6(display, 1), // not sure why, but always send 1 as the version
          destroyed{std::make_shared<bool>(false)},
          shell{shell},
          seat{seat},
          display{display}
    {
    }

    ~XdgShellV6()
    {
        *destroyed = true;
    }

    void destroy(struct wl_client* /*client*/, struct wl_resource* /*resource*/) override
    {
        // TODO: do we need to destroy this resource somehow?
    }

    void create_positioner(struct wl_client* /*client*/, struct wl_resource* /*resource*/, uint32_t /*id*/) override
    {
        // TODO
    }

    void get_xdg_surface(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* surface) override
    {
        new XdgSurfaceV6(display, client, resource, id, surface, shell, seat);
    }

    void pong(struct wl_client* /*client*/, struct wl_resource* /*resource*/, uint32_t /*serial*/) override
    {
        // this is just to test responsiveness, fine to ignore for now
    }

private:
    std::shared_ptr<bool> const destroyed;
    std::shared_ptr<mf::Shell> const shell;
    WlSeat& seat;
    wl_display* display;
};
}
}

namespace
{
int halt_eventloop(int fd, uint32_t /*mask*/, void* data)
{
    auto display = reinterpret_cast<wl_display*>(data);
    wl_display_terminate(display);

    eventfd_t ignored;
    if (eventfd_read(fd, &ignored) < 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{
           errno,
           std::system_category(),
           "Failed to consume pause event notification"}));
    }
    return 0;
}
}

namespace
{
void cleanup_display(wl_display *display)
{
    wl_display_flush_clients(display);
    wl_display_destroy(display);
}

class WaylandExecutor : public mir::Executor
{
public:
    void spawn (std::function<void ()>&& work) override
    {
        {
            std::lock_guard<std::recursive_mutex> lock{mutex};
            workqueue.emplace_back(std::move(work));
        }
        if (auto err = eventfd_write(notify_fd, 1))
        {
            BOOST_THROW_EXCEPTION((std::system_error{err, std::system_category(), "eventfd_write failed to notify event loop"}));
        }
    }

    /**
     * Get an Executor which dispatches onto a wl_event_loop
     *
     * \note    The executor may outlive the wl_event_loop, but no tasks will be dispatched
     *          after the wl_event_loop is destroyed.
     *
     * \param [in]  loop    The event loop to dispatch on
     * \return              An Executor that queues onto the wl_event_loop
     */
    static std::shared_ptr<mir::Executor> executor_for_event_loop(wl_event_loop* loop)
    {
        if (auto notifier = wl_event_loop_get_destroy_listener(loop, &on_display_destruction))
        {
            DestructionShim* shim;
            shim = wl_container_of(notifier, shim, destruction_listener);

            return shim->executor;
        }
        else
        {
            auto const executor = std::shared_ptr<WaylandExecutor>{new WaylandExecutor{loop}};
            auto shim = std::make_unique<DestructionShim>(executor);

            shim->destruction_listener.notify = &on_display_destruction;
            wl_event_loop_add_destroy_listener(loop, &(shim.release())->destruction_listener);

            return executor;
        }
    }

private:
    WaylandExecutor(wl_event_loop* loop)
        : notify_fd{eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE | EFD_NONBLOCK)},
          notify_source{wl_event_loop_add_fd(loop, notify_fd, WL_EVENT_READABLE, &on_notify, this)}
    {
        if (notify_fd == mir::Fd::invalid)
        {
            BOOST_THROW_EXCEPTION((std::system_error{
                errno,
                std::system_category(),
                "Failed to create IPC pause notification eventfd"}));
        }
    }

    std::function<void()> get_work()
    {
        std::lock_guard<std::recursive_mutex> lock{mutex};
        if (!workqueue.empty())
        {
            auto const work = std::move(workqueue.front());
            workqueue.pop_front();
            return work;
        }
        return {};
    }

    static int on_notify(int fd, uint32_t, void* data)
    {
        auto executor = static_cast<WaylandExecutor*>(data);

        eventfd_t unused;
        if (auto err = eventfd_read(fd, &unused))
        {
            mir::log_error(
                "eventfd_read failed to consume wakeup notification: %s (%i)",
                strerror(err),
                err);
        }

        while (auto work = executor->get_work())
        {
            try
            {
                work();
            }
            catch(...)
            {
                mir::log(
                    mir::logging::Severity::critical,
                    MIR_LOG_COMPONENT,
                    std::current_exception(),
                    "Exception processing Wayland event loop work item");
            }
        }

        return 0;
    }

    static void on_display_destruction(wl_listener* listener, void*)
    {
        DestructionShim* shim;
        shim = wl_container_of(listener, shim, destruction_listener);

        {
            std::lock_guard<std::recursive_mutex> lock{shim->executor->mutex};
            wl_event_source_remove(shim->executor->notify_source);
        }
        delete shim;
    }

    std::recursive_mutex mutex;
    mir::Fd const notify_fd;
    std::deque<std::function<void()>> workqueue;

    wl_event_source* const notify_source;

    struct DestructionShim
    {
        explicit DestructionShim(std::shared_ptr<WaylandExecutor> const& executor)
            : executor{executor}
        {
        }

        std::shared_ptr<WaylandExecutor> const executor;
        wl_listener destruction_listener;
    };
    static_assert(
        std::is_standard_layout<DestructionShim>::value,
        "DestructionShim must be Standard Layout for wl_container_of to be defined behaviour");
};
}

mf::WaylandConnector::WaylandConnector(
    optional_value<std::string> const& display_name,
    std::shared_ptr<mf::Shell> const& shell,
    DisplayChanger& display_config,
    std::shared_ptr<mi::InputDeviceHub> const& input_hub,
    std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<mf::SessionAuthorizer> const& session_authorizer,
    bool arw_socket)
    : display{wl_display_create(), &cleanup_display},
      pause_signal{eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE)},
      allocator{std::dynamic_pointer_cast<mg::WaylandAllocator>(allocator)}
{
    if (pause_signal == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((std::system_error{
            errno,
            std::system_category(),
            "Failed to create IPC pause notification eventfd"}));
    }

    if (!display)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to create wl_display"});
    }

    /*
     * Here be Dragons!
     *
     * Some clients expect a certain order in the publication of globals, and will
     * crash with a different order. Yay!
     *
     * So far I've only found ones which expect wl_compositor before anything else,
     * so stick that first.
     */
    auto const executor = WaylandExecutor::executor_for_event_loop(wl_display_get_event_loop(display.get()));

    compositor_global = std::make_unique<mf::WlCompositor>(
        display.get(),
        executor,
        this->allocator);
    seat_global = std::make_unique<mf::WlSeat>(display.get(), input_hub, executor);
    output_manager = std::make_unique<mf::OutputManager>(
        display.get(),
        display_config);
    shell_global = std::make_unique<mf::WlShell>(display.get(), shell, *seat_global);
    xdg_shell_global = std::make_unique<mf::XdgShellV6>(display.get(), shell, *seat_global);

    wl_display_init_shm(display.get());

    if (this->allocator)
    {
        this->allocator->bind_display(display.get());
    }
    else
    {
        mir::log_warning("No WaylandAllocator EGL support!");
    }

    char const* wayland_display = nullptr;

    if (!display_name.is_set())
    {
        wayland_display = wl_display_add_socket_auto(display.get());
    }
    else
    {
        if (!wl_display_add_socket(display.get(), display_name.value().c_str()))
        {
            wayland_display = display_name.value().c_str();
        }
    }

    if (wayland_display)
    {
        if (arw_socket)
        {
            chmod((std::string{getenv("XDG_RUNTIME_DIR")} + "/" + wayland_display).c_str(),
                  S_IRUSR|S_IWUSR| S_IRGRP|S_IWGRP | S_IROTH|S_IWOTH);
        };
    }

    auto wayland_loop = wl_display_get_event_loop(display.get());

    setup_new_client_handler(display.get(), shell, session_authorizer);

    pause_source = wl_event_loop_add_fd(wayland_loop, pause_signal, WL_EVENT_READABLE, &halt_eventloop, display.get());
}

mf::WaylandConnector::~WaylandConnector()
{
    if (dispatch_thread.joinable())
    {
        stop();
    }
    wl_event_source_remove(pause_source);
}

void mf::WaylandConnector::start()
{
    dispatch_thread = std::thread{wl_display_run, display.get()};
}

void mf::WaylandConnector::stop()
{
    if (eventfd_write(pause_signal, 1) < 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{
            errno,
            std::system_category(),
            "Failed to send IPC eventloop pause signal"}));
    }
    if (dispatch_thread.joinable())
    {
        dispatch_thread.join();
        dispatch_thread = std::thread{};
    }
    else
    {
        mir::log_warning("WaylandConnector::stop() called on not-running connector?");
    }
}

int mf::WaylandConnector::client_socket_fd() const
{
    enum { server, client, size };
    int socket_fd[size];

    char const* error = nullptr;

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, socket_fd))
    {
        error = "Could not create socket pair";
    }
    else
    {
        // TODO: Wait on the result of wl_client_create so we can throw an exception on failure.
        WaylandExecutor::executor_for_event_loop(wl_display_get_event_loop(display.get()))
            ->spawn(
                [socket = socket_fd[server], display = display.get()]()
                {
                    if (!wl_client_create(display, socket))
                    {
                        mir::log_error(
                            "Failed to create Wayland client object: %s (errno %i)",
                            strerror(errno),
                            errno);
                    }
                });
    }

    if (error)
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), error}));

    return socket_fd[client];
}

int mf::WaylandConnector::client_socket_fd(
    std::function<void(std::shared_ptr<Session> const& session)> const& /*connect_handler*/) const
{
    return -1;
}

void mf::WaylandConnector::run_on_wayland_display(std::function<void(wl_display*)> const& functor)
{
    auto executor = WaylandExecutor::executor_for_event_loop(wl_display_get_event_loop(display.get()));

    executor->spawn([display_ref = display.get(), functor]() { functor(display_ref); });
}
