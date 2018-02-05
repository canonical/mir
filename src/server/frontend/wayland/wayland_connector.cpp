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

#include "basic_surface_event_sink.h"
#include "null_event_sink.h"
#include "output_manager.h"
#include "wayland_executor.h"
#include "wlshmbuffer.h"

#include "core_generated_interfaces.h"
#include "xdg_shell_generated_interfaces.h"

#include "mir/frontend/shell.h"
#include "mir/frontend/surface.h"
#include "mir/frontend/session_credentials.h"
#include "mir/frontend/session_authorizer.h"

#include "mir/compositor/buffer_stream.h"

#include "mir/frontend/session.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/surface.h"

#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/wayland_allocator.h"

#include "mir/renderer/gl/texture_target.h"
#include "mir/frontend/buffer_stream_id.h"

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

#include <functional>
#include <type_traits>

#include <xkbcommon/xkbcommon.h>
#include <linux/input.h>
#include <mir/log.h>
#include <cstring>

#include "mir/fd.h"

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
namespace
{
struct ClientPrivate
{
    ClientPrivate(std::shared_ptr<mf::Session> const& session, mf::Shell* shell)
        : session{session},
          shell{shell}
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
        std::make_shared<NullEventSink>());

    auto client_context = new ClientPrivate{session, construction_context->shell.get()};
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

struct WlMirWindow
{
    virtual void new_buffer_size(geometry::Size const& buffer_size) = 0;
    virtual void commit() = 0;
    virtual void visiblity(bool visible) = 0;
    virtual void destroy() = 0;
    virtual ~WlMirWindow() = default;
};

struct NullWlMirWindow : WlMirWindow
{
    void new_buffer_size(geom::Size const& ) {}
    void commit() {}
    void visiblity(bool ) {}
    void destroy() {}
} nullwlmirwindow;

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

    void set_role(WlMirWindow* role_)
    {
        role = role_;
    }

    mf::BufferStreamId stream_id;
    std::shared_ptr<mf::BufferStream> stream;
    mf::SurfaceId surface_id;       // ID of any associated surface

    std::shared_ptr<bool> destroyed_flag() const
    {
        return destroyed;
    }

private:
    std::shared_ptr<mg::WaylandAllocator> const allocator;
    std::shared_ptr<mir::Executor> const executor;

    WlMirWindow* role = &nullwlmirwindow;

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

WlSurface* get_wlsurface(wl_resource* surface)
{
    auto* tmp = wl_resource_get_user_data(surface);
    return static_cast<WlSurface*>(static_cast<wayland::Surface*>(tmp));
}

void WlSurface::destroy()
{
    *destroyed = true;
    role->destroy();
    wl_resource_destroy(resource);
}

void WlSurface::attach(std::experimental::optional<wl_resource*> const& buffer, int32_t x, int32_t y)
{
    if (x != 0 || y != 0)
    {
        mir::log_warning("Client requested unimplemented non-zero attach offset. Rendering will be incorrect.");
    }

    role->visiblity(!!buffer);

    pending_buffer = buffer.value_or(nullptr);
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
        role->new_buffer_size(mir_buffer->size());
        stream->submit_buffer(mir_buffer);

        pending_buffer = nullptr;
    }
    role->commit();
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
                    target_window_destroyed = get_wlsurface(target)->destroyed_flag(),
                    focussed = mir_window_event_get_attribute_value(event),
                    this
                ]()
                {
                    if (*target_window_destroyed)
                        return;

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
        *destroyed = true;
        on_destroy(this);
    }

    void handle_event(MirInputEvent const* event, wl_resource* target)
    {
        executor->spawn(run_unless(
            destroyed,
            [
                ev = mcl::Event{mir_input_event_get_event(event)},
                target,
                target_window_destroyed = get_wlsurface(target)->destroyed_flag(),
                this
            ]()
            {
                if (*target_window_destroyed)
                    return;

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
            [
                ev = mcl::Event{mir_input_event_get_event(event)},
                target = target,
                target_window_destroyed = get_wlsurface(target)->destroyed_flag(),
                this
            ]()
            {
                if (*target_window_destroyed)
                    return;

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

class WlSeat : public BasicWlSeat
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

    void handle_pointer_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const override;
    void handle_keyboard_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const override;
    void handle_touch_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const override;
    void handle_event(wl_client* client, MirKeymapEvent const* keymap_event, wl_resource* target) const override;
    void handle_event(wl_client* client, MirWindowEvent const* window_event, wl_resource* target) const override;

    void spawn(std::function<void()>&& work) override
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

void WlSeat::handle_pointer_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const
{
    pointer[client].handle_event(input_event, target);
}

void WlSeat::handle_keyboard_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const
{
    keyboard[client].handle_event(input_event, target);
}

void WlSeat::handle_touch_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const
{
    touch[client].handle_event(input_event, target);
}

void WlSeat::handle_event(wl_client* client, MirKeymapEvent const* keymap_event, wl_resource* target) const
{
    keyboard[client].handle_event(keymap_event, target);
}

void WlSeat::handle_event(wl_client* client, MirWindowEvent const* window_event, wl_resource* target) const
{
    keyboard[client].handle_event(window_event, target);
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

class SurfaceEventSink : public BasicSurfaceEventSink
{
public:
    SurfaceEventSink(WlSeat* seat, wl_client* client, wl_resource* target, wl_resource* event_sink,
        std::shared_ptr<bool> const& destroyed) :
        BasicSurfaceEventSink{seat, client, target, event_sink},
        destroyed{destroyed}
    {
    }

    void send_resize(geometry::Size const& new_size) const override;

private:
    std::shared_ptr<bool> const destroyed;
};

void SurfaceEventSink::send_resize(geometry::Size const& new_size) const
{
    if (window_size != new_size)
    {
        seat->spawn(run_unless(
            destroyed,
            [event_sink= event_sink, width = new_size.width.as_int(), height = new_size.height.as_int()]()
            {
                wl_shell_surface_send_configure(event_sink, WL_SHELL_SURFACE_RESIZE_NONE, width, height);
            }));
    }
}

class WlAbstractMirWindow : public WlMirWindow
{
public:
    WlAbstractMirWindow(wl_client* client, wl_resource* surface, wl_resource* event_sink,
        std::shared_ptr<mf::Shell> const& shell) :
        destroyed{std::make_shared<bool>(false)},
        client{client},
        surface{surface},
        event_sink{event_sink},
        shell{shell}
    {
    }

    ~WlAbstractMirWindow() override
    {
        *destroyed = true;
        if (surface_id.as_value())
        {
            if (auto session = session_for_client(client))
            {
                shell->destroy_surface(session, surface_id);
            }

            surface_id = {};
        }
    }

protected:
    std::shared_ptr<bool> const destroyed;
    wl_client* const client;
    wl_resource* const surface;
    wl_resource* const event_sink;
    std::shared_ptr<mf::Shell> const shell;
    std::shared_ptr<BasicSurfaceEventSink> sink;

    ms::SurfaceCreationParameters params = ms::SurfaceCreationParameters().of_type(mir_window_type_freestyle);
    mf::SurfaceId surface_id;

    shell::SurfaceSpecification& spec()
    {
        if (!pending_changes)
            pending_changes = std::make_unique<shell::SurfaceSpecification>();

        return *pending_changes;
    }

private:
    geom::Size latest_buffer_size;
    std::unique_ptr<shell::SurfaceSpecification> pending_changes;

    void commit() override
    {
        auto const session = session_for_client(client);

        if (surface_id.as_value())
        {
            auto const surface = get_surface_for_id(session, surface_id);

            if (surface->size() != latest_buffer_size)
            {
                sink->latest_resize(latest_buffer_size);
                auto& new_size_spec = spec();
                new_size_spec.width = latest_buffer_size.width;
                new_size_spec.height = latest_buffer_size.height;
            }

            if (pending_changes)
                shell->modify_surface(session, surface_id, *pending_changes);

            pending_changes.reset();
            return;
        }

        // Until we've seen a buffer we don't create a surface
        if (latest_buffer_size == geom::Size{})
            return;

        auto* const mir_surface = get_wlsurface(surface);
        if (params.size == geom::Size{}) params.size = latest_buffer_size;
        params.content_id = mir_surface->stream_id;
        surface_id = shell->create_surface(session, params, sink);
        mir_surface->surface_id = surface_id;

        // The shell isn't guaranteed to respect the requested size
        auto const window = session->get_surface(surface_id);
        sink->send_resize(window->client_size());
    }

    void new_buffer_size(geometry::Size const& buffer_size) override
    {
        latest_buffer_size = buffer_size;

        // Sometimes, when using xdg-shell, qterminal creates an insanely tall buffer
        if (latest_buffer_size.height > geometry::Height{10000})
        {
            log_warning("Insane buffer height sanitized: latest_buffer_size.height = %d (was %d)",
                 1000, latest_buffer_size.height.as_int());
            latest_buffer_size.height = geometry::Height{1000};
        }
    }

    void visiblity(bool visible) override
    {
        if (!surface_id.as_value())
            return;

        auto const session = session_for_client(client);

        if (get_surface_for_id(session, surface_id)->visible() == visible)
            return;

        spec().state = visible ? mir_window_state_restored : mir_window_state_hidden;
    }
};

class WlShellSurface  : public wayland::ShellSurface, WlAbstractMirWindow
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
        WlAbstractMirWindow{client, surface, resource, shell}
    {
        // We can't pass this to the WlAbstractMirWindow constructor as it needs creating *after* destroyed
        sink = std::make_shared<SurfaceEventSink>(&seat, client, surface, event_sink, destroyed);
    }

    ~WlShellSurface() override
    {
        auto* const mir_surface = get_wlsurface(surface);
        mir_surface->set_role(&nullwlmirwindow);
    }

protected:
    void destroy() override
    {
        wl_resource_destroy(resource);
    }

    void set_toplevel() override
    {
        auto* const mir_surface = get_wlsurface(surface);

        mir_surface->set_role(this);
    }

    void set_transient(
        struct wl_resource* parent,
        int32_t x,
        int32_t y,
        uint32_t flags) override
    {
        auto const session = session_for_client(client);
        auto& parent_surface = *get_wlsurface(parent);

        if (surface_id.as_value())
        {
            auto& mods = spec();
            mods.parent_id = parent_surface.surface_id;
            mods.aux_rect = geom::Rectangle{{x, y}, {}};
            mods.surface_placement_gravity = mir_placement_gravity_northwest;
            mods.aux_rect_placement_gravity = mir_placement_gravity_southeast;
            mods.placement_hints = mir_placement_hints_slide_x;
            mods.aux_rect_placement_offset_x = 0;
            mods.aux_rect_placement_offset_y = 0;
        }
        else
        {
            if (flags & WL_SHELL_SURFACE_TRANSIENT_INACTIVE)
                params.type = mir_window_type_gloss;
            params.parent_id = parent_surface.surface_id;
            params.aux_rect = geom::Rectangle{{x, y}, {}};
            params.surface_placement_gravity = mir_placement_gravity_northwest;
            params.aux_rect_placement_gravity = mir_placement_gravity_southeast;
            params.placement_hints = mir_placement_hints_slide_x;
            params.aux_rect_placement_offset_x = 0;
            params.aux_rect_placement_offset_y = 0;

            auto* const mir_surface = get_wlsurface(surface);
            mir_surface->set_role(this);
        }
    }

    void set_fullscreen(
        uint32_t /*method*/,
        uint32_t /*framerate*/,
        std::experimental::optional<struct wl_resource*> const& output) override
    {
        if (surface_id.as_value())
        {
            auto& mods = spec();
            mods.state = mir_window_state_fullscreen;
            if (output)
            {
                // TODO{alan_g} mods.output_id = DisplayConfigurationOutputId_from(output)
            }
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
        uint32_t flags) override
    {
        auto const session = session_for_client(client);
        auto& parent_surface = *get_wlsurface(parent);

        if (surface_id.as_value())
        {
            auto& mods = spec();
            mods.parent_id = parent_surface.surface_id;
            mods.aux_rect = geom::Rectangle{{x, y}, {}};
            mods.surface_placement_gravity = mir_placement_gravity_northwest;
            mods.aux_rect_placement_gravity = mir_placement_gravity_southeast;
            mods.placement_hints = mir_placement_hints_slide_x;
            mods.aux_rect_placement_offset_x = 0;
            mods.aux_rect_placement_offset_y = 0;
        }
        else
        {
            if (flags & WL_SHELL_SURFACE_TRANSIENT_INACTIVE)
                params.type = mir_window_type_gloss;

            params.parent_id = parent_surface.surface_id;
            params.aux_rect = geom::Rectangle{{x, y}, {}};
            params.surface_placement_gravity = mir_placement_gravity_northwest;
            params.aux_rect_placement_gravity = mir_placement_gravity_southeast;
            params.placement_hints = mir_placement_hints_slide_x;
            params.aux_rect_placement_offset_x = 0;
            params.aux_rect_placement_offset_y = 0;

            auto* const mir_surface = get_wlsurface(surface);
            mir_surface->set_role(this);
        }
    }

    void set_maximized(std::experimental::optional<struct wl_resource*> const& output) override
    {
        if (surface_id.as_value())
        {
            auto& mods = spec();
            mods.state = mir_window_state_maximized;
            if (output)
            {
                // TODO{alan_g} mods.output_id = DisplayConfigurationOutputId_from(output)
            }
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
            spec().name = title;
        }
        else
        {
            params.name = title;
        }
    }

    void pong(uint32_t /*serial*/) override
    {
    }

    void move(struct wl_resource* /*seat*/, uint32_t /*serial*/) override
    {
    }

    void resize(struct wl_resource* /*seat*/, uint32_t /*serial*/, uint32_t /*edges*/) override
    {
    }

    void set_class(std::string const& /*class_*/) override
    {
    }

    using WlAbstractMirWindow::client;
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

struct XdgPositionerV6 : wayland::XdgPositionerV6
{
    XdgPositionerV6(struct wl_client* client, struct wl_resource* parent, uint32_t id) :
        wayland::XdgPositionerV6(client, parent, id)
    {
    }

    void destroy() override
    {
        wl_resource_destroy(resource);
    }

    void set_size(int32_t width, int32_t height) override
    {
        size = geom::Size{width, height};
    }

    void set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height) override
    {
        aux_rect = geom::Rectangle{{x, y}, {width, height}};
    }

    void set_anchor(uint32_t anchor) override
    {
        MirPlacementGravity placement = mir_placement_gravity_center;

        if (anchor & ZXDG_POSITIONER_V6_ANCHOR_TOP)
            placement = MirPlacementGravity(placement | mir_placement_gravity_north);

        if (anchor & ZXDG_POSITIONER_V6_ANCHOR_BOTTOM)
            placement = MirPlacementGravity(placement | mir_placement_gravity_south);

        if (anchor & ZXDG_POSITIONER_V6_ANCHOR_LEFT)
            placement = MirPlacementGravity(placement | mir_placement_gravity_west);

        if (anchor & ZXDG_POSITIONER_V6_ANCHOR_RIGHT)
            placement = MirPlacementGravity(placement | mir_placement_gravity_east);

        surface_placement_gravity = placement;
    }

    void set_gravity(uint32_t gravity) override
    {
        MirPlacementGravity placement = mir_placement_gravity_center;

        if (gravity & ZXDG_POSITIONER_V6_GRAVITY_TOP)
            placement = MirPlacementGravity(placement | mir_placement_gravity_north);

        if (gravity & ZXDG_POSITIONER_V6_GRAVITY_BOTTOM)
            placement = MirPlacementGravity(placement | mir_placement_gravity_south);

        if (gravity & ZXDG_POSITIONER_V6_GRAVITY_LEFT)
            placement = MirPlacementGravity(placement | mir_placement_gravity_west);

        if (gravity & ZXDG_POSITIONER_V6_GRAVITY_RIGHT)
            placement = MirPlacementGravity(placement | mir_placement_gravity_east);

        aux_rect_placement_gravity = placement;
    }

    void set_constraint_adjustment(uint32_t constraint_adjustment) override
    {
        (void)constraint_adjustment;
        // TODO
    }

    void set_offset(int32_t x, int32_t y) override
    {
        aux_rect_placement_offset_x = x;
        aux_rect_placement_offset_y = y;
    }

    optional_value<geometry::Size> size;
    optional_value<geometry::Rectangle> aux_rect;
    optional_value<MirPlacementGravity> surface_placement_gravity;
    optional_value<MirPlacementGravity> aux_rect_placement_gravity;
    optional_value<int> aux_rect_placement_offset_x;
    optional_value<int> aux_rect_placement_offset_y;
};

struct XdgSurfaceV6;

struct XdgToplevelV6 : wayland::XdgToplevelV6
{
    XdgToplevelV6(struct wl_client* client, struct wl_resource* parent, uint32_t id,
        std::shared_ptr<mf::Shell> const& shell, XdgSurfaceV6* self);

    void destroy() override
    {
        wl_resource_destroy(resource);
    }

    void set_parent(std::experimental::optional<struct wl_resource*> const& parent) override;

    void set_title(std::string const& title) override;

    void set_app_id(std::string const& /*app_id*/) override
    {
        // Logically this sets the session name, but Mir doesn't allow this (currently) and
        // allowing e.g. "session_for_client(client)->name(app_id);" would break the libmirserver ABI
    }

    void show_window_menu(struct wl_resource* seat, uint32_t serial, int32_t x, int32_t y) override
    {
        (void)seat, (void)serial, (void)x, (void)y;
        // TODO
    }

    void move(struct wl_resource* seat, uint32_t serial) override
    {
        (void)seat, (void)serial;
        // TODO
    }

    void resize(struct wl_resource* seat, uint32_t serial, uint32_t edges) override
    {
        (void)seat, (void)serial, (void)edges;
        // TODO
    }

    void set_max_size(int32_t width, int32_t height) override;

    void set_min_size(int32_t width, int32_t height) override;

    void set_maximized() override;

    void unset_maximized() override;

    void set_fullscreen(std::experimental::optional<struct wl_resource*> const& output) override
    {
        (void)output;
        // TODO
    }

    void unset_fullscreen() override
    {
        // TODO
    }

    void set_minimized() override
    {
        // TODO
    }

private:
    XdgToplevelV6* get_xdgtoplevel(wl_resource* surface) const
    {
        auto* tmp = wl_resource_get_user_data(surface);
        return static_cast<XdgToplevelV6*>(static_cast<wayland::XdgToplevelV6*>(tmp));
    }

    std::shared_ptr<mf::Shell> const shell;
    XdgSurfaceV6* const self;
};

struct XdgPopupV6 : wayland::XdgPopupV6
{
    XdgPopupV6(struct wl_client* client, struct wl_resource* parent, uint32_t id) :
        wayland::XdgPopupV6(client, parent, id)
    {
    }

    void grab(struct wl_resource* seat, uint32_t serial) override
    {
        (void)seat, (void)serial;
        // TODO
    }

    void destroy() override
    {
        wl_resource_destroy(resource);
    }
};

class XdgSurfaceV6EventSink : public BasicSurfaceEventSink
{
public:
    using BasicSurfaceEventSink::BasicSurfaceEventSink;

    XdgSurfaceV6EventSink(WlSeat* seat, wl_client* client, wl_resource* target, wl_resource* event_sink,
        std::shared_ptr<bool> const& destroyed) :
        BasicSurfaceEventSink(seat, client, target, event_sink),
        destroyed{destroyed}
    {
        auto const serial = wl_display_next_serial(wl_client_get_display(client));
        post_configure(serial);
    }

    void send_resize(geometry::Size const& new_size) const override
    {
        if (window_size != new_size)
        {
            auto const serial = wl_display_next_serial(wl_client_get_display(client));
            notify_resize(new_size);
            post_configure(serial);
        }
    }

    std::function<void(geometry::Size const& new_size)> notify_resize = [](auto){};

private:
    void post_configure(int serial) const
    {
        seat->spawn(run_unless(destroyed, [event_sink= event_sink, serial]()
            {
                wl_resource_post_event(event_sink, 0, serial);
            }));
    }

    std::shared_ptr<bool> const destroyed;
};

struct XdgSurfaceV6 : wayland::XdgSurfaceV6, WlAbstractMirWindow
{
    XdgSurfaceV6* get_xdgsurface(wl_resource* surface) const
    {
        auto* tmp = wl_resource_get_user_data(surface);
        return static_cast<XdgSurfaceV6*>(static_cast<wayland::XdgSurfaceV6*>(tmp));
    }

    XdgSurfaceV6(wl_client* client,
        wl_resource* parent,
        uint32_t id,
        wl_resource* surface,
        std::shared_ptr<mf::Shell> const& shell,
        WlSeat& seat) :
        wayland::XdgSurfaceV6(client, parent, id),
        WlAbstractMirWindow{client, surface, resource, shell},
        parent{parent},
        shell{shell},
        sink{std::make_shared<XdgSurfaceV6EventSink>(&seat, client, surface, resource, destroyed)}
    {
        WlAbstractMirWindow::sink = sink;
    }

    ~XdgSurfaceV6() override
    {
        auto* const mir_surface = get_wlsurface(surface);
        mir_surface->set_role(&nullwlmirwindow);
    }

    void destroy() override
    {
        wl_resource_destroy(resource);
    }

    void get_toplevel(uint32_t id) override
    {
        new XdgToplevelV6{client, parent, id, shell, this};
        auto* const mir_surface = get_wlsurface(surface);
        mir_surface->set_role(this);
    }

    void get_popup(uint32_t id, struct wl_resource* parent, struct wl_resource* positioner) override
    {
        auto* tmp = wl_resource_get_user_data(positioner);
        auto const* const pos =  static_cast<XdgPositionerV6*>(static_cast<wayland::XdgPositionerV6*>(tmp));

        auto const session = session_for_client(client);
        auto& parent_surface = *get_xdgsurface(parent);

        params.type = mir_window_type_freestyle;
        params.parent_id = parent_surface.surface_id;
        if (pos->size.is_set()) params.size = pos->size.value();
        params.aux_rect = pos->aux_rect;
        params.surface_placement_gravity = pos->surface_placement_gravity;
        params.aux_rect_placement_gravity = pos->aux_rect_placement_gravity;
        params.aux_rect_placement_offset_x = pos->aux_rect_placement_offset_x;
        params.aux_rect_placement_offset_y = pos->aux_rect_placement_offset_y;
        params.placement_hints = mir_placement_hints_slide_any;

        new XdgPopupV6{client, parent, id};
        auto* const mir_surface = get_wlsurface(surface);
        mir_surface->set_role(this);
    }

    void set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height) override
    {
        geom::Rectangle const input_region{{x, y}, {width, height}};

        if (surface_id.as_value())
        {
            spec().input_shape = {input_region};
        }
        else
        {
            params.input_shape = {input_region};
        }
    }

    void ack_configure(uint32_t serial) override
    {
        (void)serial;
        // TODO
    }

    void set_parent(optional_value<SurfaceId> parent_id);
    void set_title(std::string const& title);
    void set_max_size(int32_t width, int32_t height);
    void set_min_size(int32_t width, int32_t height);
    void set_maximized();
    void unset_maximized();

    using WlAbstractMirWindow::client;
    using WlAbstractMirWindow::params;
    using WlAbstractMirWindow::surface_id;
    struct wl_resource* const parent;
    std::shared_ptr<mf::Shell> const shell;
    std::shared_ptr<XdgSurfaceV6EventSink> const sink;
};

XdgToplevelV6::XdgToplevelV6(struct wl_client* client, struct wl_resource* parent, uint32_t id,
    std::shared_ptr<mf::Shell> const& shell, XdgSurfaceV6* self) :
    wayland::XdgToplevelV6(client, parent, id),
    shell{shell},
    self{self}
{
    self->sink->notify_resize = [this](geom::Size const& new_size)
        {
            wl_array states;
            wl_array_init(&states);

            zxdg_toplevel_v6_send_configure(resource, new_size.width.as_int(), new_size.height.as_int(), &states);
        };
}


void XdgSurfaceV6::set_title(std::string const& title)
{
    if (surface_id.as_value())
    {
        spec().name = title;
    }
    else
    {
        params.name = title;
    }
}


void XdgSurfaceV6::set_parent(optional_value<SurfaceId> parent_id)
{
    if (surface_id.as_value())
    {
        spec().parent_id = parent_id;
    }
    else
    {
        params.parent_id = parent_id;
    }
}

void XdgSurfaceV6::set_max_size(int32_t width, int32_t height)
{
    if (surface_id.as_value())
    {
        if (width == 0) width = std::numeric_limits<int>::max();
        if (height == 0) height = std::numeric_limits<int>::max();

        auto& mods = spec();
        mods.max_width = geom::Width{width};
        mods.max_height = geom::Height{height};
    }
    else
    {
        if (width == 0)
        {
            if (params.max_width.is_set())
                params.max_width.consume();
        }
        else
            params.max_width = geom::Width{width};

        if (height == 0)
        {
            if (params.max_height.is_set())
                params.max_height.consume();
        }
        else
            params.max_height = geom::Height{height};
    }
}

void XdgSurfaceV6::set_min_size(int32_t width, int32_t height)
{
    if (surface_id.as_value())
    {
        auto& mods = spec();
        mods.min_width = geom::Width{width};
        mods.min_height = geom::Height{height};
    }
    else
    {
        params.min_width = geom::Width{width};
        params.min_height = geom::Height{height};
    }
}

void XdgSurfaceV6::set_maximized()
{
    if (surface_id.as_value())
    {
        spec().state = mir_window_state_maximized;
    }
    else
    {
        params.state = mir_window_state_maximized;
    }
}

void XdgSurfaceV6::unset_maximized()
{
    if (surface_id.as_value())
    {
        spec().state = mir_window_state_restored;
    }
    else
    {
        params.state = mir_window_state_restored;
    }
}


void XdgToplevelV6::set_parent(std::experimental::optional<struct wl_resource*> const& parent)
{
    if (parent && parent.value())
    {
        self->set_parent(get_xdgtoplevel(parent.value())->self->surface_id);
    }
    else
    {
        self->set_parent({});
    }

}

void XdgToplevelV6::set_title(std::string const& title)
{
    self->set_title(title);
}

void XdgToplevelV6::set_max_size(int32_t width, int32_t height)
{
    self->set_max_size(width, height);
}

void XdgToplevelV6::set_min_size(int32_t width, int32_t height)
{
    self->set_min_size(width, height);
}

void XdgToplevelV6::set_maximized()
{
    self->set_maximized();
}

void XdgToplevelV6::unset_maximized()
{
    self->unset_maximized();
}

struct XdgShellV6 : wayland::XdgShellV6
{
    XdgShellV6(
        struct wl_display* display,
        std::shared_ptr<mf::Shell> const shell,
        WlSeat& seat) :
        wayland::XdgShellV6(display, 1),
        shell{shell},
        seat{seat}
    {
    }

    void destroy(struct wl_client* client, struct wl_resource* resource) override
    {
        (void)client, (void)resource;
        // TODO
    }

    void create_positioner(struct wl_client* client, struct wl_resource* resource, uint32_t id) override
    {
        new XdgPositionerV6{client, resource, id};
    }

    void get_xdg_surface(
        struct wl_client* client,
        struct wl_resource* resource,
        uint32_t id,
        struct wl_resource* surface) override
    {
        new XdgSurfaceV6{client, resource, id, surface, shell, seat};
    }

    void pong(struct wl_client* client, struct wl_resource* resource, uint32_t serial) override
    {
        (void)client, (void)resource, (void)serial;
        // TODO
    }

private:
    std::shared_ptr<mf::Shell> const shell;
    WlSeat& seat;
};

struct DataDevice : wayland::DataDevice
{
    DataDevice(struct wl_client* client, struct wl_resource* parent, uint32_t id) :
        wayland::DataDevice(client, parent, id)
    {
    }

    void start_drag(
        std::experimental::optional<struct wl_resource*> const& source, struct wl_resource* origin,
        std::experimental::optional<struct wl_resource*> const& icon, uint32_t serial) override
    {
        (void)source, (void)origin, (void)icon, (void)serial;
    }

    void set_selection(std::experimental::optional<struct wl_resource*> const& source, uint32_t serial) override
    {
        (void)source, (void)serial;
    }

    void release() override
    {
    }
};

struct DataDeviceManager : wayland::DataDeviceManager
{
    DataDeviceManager(struct wl_display* display) :
        wayland::DataDeviceManager(display, 3)
    {
    }

    void create_data_source(struct wl_client* client, struct wl_resource* resource, uint32_t id) override
    {
        (void)client, (void)resource, (void)id;
    }

    void get_data_device(
        struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* seat) override
    {
        (void)seat;
        new DataDevice{client, resource, id};
    }
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
    auto const executor = executor_for_event_loop(wl_display_get_event_loop(display.get()));

    compositor_global = std::make_unique<mf::WlCompositor>(
        display.get(),
        executor,
        this->allocator);
    seat_global = std::make_unique<mf::WlSeat>(display.get(), input_hub, executor);
    output_manager = std::make_unique<mf::OutputManager>(
        display.get(),
        display_config);
    shell_global = std::make_unique<mf::WlShell>(display.get(), shell, *seat_global);
    data_device_manager_global = std::make_unique<DataDeviceManager>(display.get());

    // The XDG shell support is currently too flaky to enable by default
    if (getenv("MIR_EXPERIMENTAL_XDG_SHELL"))
        xdg_shell_global = std::make_unique<XdgShellV6>(display.get(), shell, *seat_global);

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
        executor_for_event_loop(wl_display_get_event_loop(display.get()))
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
    auto executor = executor_for_event_loop(wl_display_get_event_loop(display.get()));

    executor->spawn([display_ref = display.get(), functor]() { functor(display_ref); });
}
