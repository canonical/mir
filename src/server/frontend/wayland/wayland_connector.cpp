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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "wayland_connector.h"

#include "core_generated_interfaces.h"

#include "mir_server_protocol.h"
#include "mir_generated_interfaces.h"

#include "mir/frontend/shell.h"

#include "mir/compositor/buffer_stream.h"

#include "mir/frontend/session.h"
#include "mir/frontend/event_sink.h"
#include "mir/scene/surface_creation_parameters.h"

#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/display_configuration.h"

#include "../../scene/global_event_sender.h"
#include "../../scene/mediating_display_changer.h"

#include <system_error>
#include <sys/eventfd.h>
#include <wayland-server-core.h>
#include <unordered_map>
#include <boost/throw_exception.hpp>

#include <future>
#include <functional>
#include <type_traits>

#include <algorithm>
#include <iostream>
#include <mir/log.h>

#include "mir/fd.h"
#include "../../../platforms/common/server/shm_buffer.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace ms = mir::scene;

namespace
{
mf::BufferStreamId create_buffer_stream(mf::Session& session)
{
    mir::geometry::Size const dummy { 10, 10 };
    mg::BufferProperties const properties(dummy, mir_pixel_format_argb_8888, mg::BufferUsage::software);
    return session.create_buffer_stream(properties);
}
}

namespace mir
{
namespace frontend
{

class ClientDepository
{
private:
    friend class ClientShutdownContext;
    class ClientShutdownContext
    {
    public:
        ClientShutdownContext(
            wl_client* client,
            ClientDepository* parent,
            std::function<void(std::shared_ptr<mf::Session> const&)> const& on_client_death)
            : depository{parent},
              on_client_death{std::make_unique<std::function<void(std::shared_ptr<mf::Session> const&)>>(on_client_death)}
        {
            listener.notify = &ClientShutdownContext::on_client_disconnect;
            wl_client_add_destroy_listener(client, &listener);
        }

    private:
        static void on_client_disconnect(wl_listener* listener, void* data)
        {
            ClientShutdownContext* context;
            auto client = reinterpret_cast<wl_client*>(data);

            context = reinterpret_cast<ClientShutdownContext*>(wl_container_of(listener, context, listener));

            auto session = context->depository->session_for_client(client);
            (*context->on_client_death)(session);
            context->depository->remove_client_session(client);

            delete context;
        }


        ClientDepository* const depository;
        std::unique_ptr<std::function<void(std::shared_ptr<mf::Session> const&)>> on_client_death;
        wl_listener listener;
    };

public:
    std::shared_ptr<mf::Session> session_for_client(wl_client* client)
    {
        return client_sessions.at(client).lock();
    }

    void add_client_session(
        wl_client* client,
        std::weak_ptr<mf::Session> const& session,
        std::function<void(std::shared_ptr<mf::Session> const&)> on_client_disconnect)
    {
        client_sessions[client] = session;
        new ClientShutdownContext{client, this, on_client_disconnect};
    }

private:
    void remove_client_session(wl_client* client)
    {
        client_sessions.erase(client);
    }

    std::unordered_map<wl_client*, std::weak_ptr<mf::Session>> client_sessions;
};


class WlSurface : public wayland::Surface
{
public:
    WlSurface(
        mf::Session& session,
        wl_client* client,
        wl_resource* parent,
        uint32_t id)
        : Surface(client, parent, id),
          stream_id{create_buffer_stream(session)},
          stream{std::dynamic_pointer_cast<mc::BufferStream>(session.get_buffer_stream(stream_id))},
          session{session}
    {
        mir::log_info("Created WlSurface");
    }

    mf::BufferStreamId const stream_id;
    std::shared_ptr<compositor::BufferStream> const stream;
private:
    mf::Session& session;

    graphics::BufferID working_buffer;
    graphics::BufferID submitted_buffer;

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
    delete this;
}

void WlSurface::attach(std::experimental::optional<wl_resource*> const& buffer, int32_t x, int32_t y)
{
    if (x != 0 || y != 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Non-zero buffer offsets are unimplemented"));
    }

    if (!buffer)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Setting null buffer is unimplemented"));
    }

    auto shm_buffer = wl_shm_buffer_get(*buffer);
    if (!shm_buffer)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Non-shm buffers are unimplemented"));
    }

    wl_shm_buffer_begin_access(shm_buffer);
    if (wl_shm_buffer_get_height(shm_buffer) != stream->stream_size().height.as_int() ||
        wl_shm_buffer_get_width(shm_buffer) != stream->stream_size().width.as_int())
    {
        stream->resize({wl_shm_buffer_get_height(shm_buffer), wl_shm_buffer_get_width(shm_buffer)});
        stream->drop_old_buffers();
    }

    std::swap(working_buffer, submitted_buffer);

    if (!working_buffer.as_value())
        working_buffer = session.create_buffer(stream->stream_size(), mir_pixel_format_argb_8888);
    auto const mir_buffer = session.get_buffer(working_buffer);

    auto const shm_size = wl_shm_buffer_get_height(shm_buffer) *
                          wl_shm_buffer_get_stride(shm_buffer) * 4;
    auto const data = reinterpret_cast<uint8_t const*>(wl_shm_buffer_get_data(shm_buffer));

    dynamic_cast<mir::graphics::common::ShmBuffer&>(*mir_buffer).write(data, shm_size);
    wl_shm_buffer_end_access(shm_buffer);

    wl_buffer_send_release(*buffer);

    stream->submit_buffer(mir_buffer);
}

void WlSurface::damage(int32_t x, int32_t y, int32_t width, int32_t height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void WlSurface::damage_buffer(int32_t x, int32_t y, int32_t width, int32_t height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void WlSurface::frame(uint32_t callback)
{
    (void)callback;
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
}

void WlSurface::set_buffer_transform(int32_t transform)
{
    (void)transform;
}

void WlSurface::set_buffer_scale(int32_t scale)
{
    (void)scale;
}

class WlCompositor : public wayland::Compositor
{
public:
    WlCompositor(
        std::shared_ptr<ClientDepository> const& depository,
        struct wl_display* display)
        : Compositor(display, 3),
          depository{depository}
    {
    }

private:
    std::shared_ptr<ClientDepository> const depository;

    void create_surface(wl_client* client, wl_resource* resource, uint32_t id) override;
    void create_region(wl_client* client, wl_resource* resource, uint32_t id) override;
};

void WlCompositor::create_surface(wl_client* client, wl_resource* resource, uint32_t id)
{
    new WlSurface{*depository->session_for_client(client), client, resource, id};
}

void WlCompositor::create_region(wl_client* client, wl_resource* resource, uint32_t id)
{
    (void)client;
    (void)resource;
    (void)id;
}

class WlPointer;
class WlTouch;

class WlKeyboard : public wayland::Keyboard
{
public:
    WlKeyboard(wl_client* client, wl_resource* parent, uint32_t id)
        : Keyboard(client, parent, id)
    {
    }

    void handle_event(MirInputEvent const* event, wl_resource* /*target*/)
    {
        std::cout << "Hello! In WlKeyboard::handle_event" << std::endl;

        int serial = 0;
        auto key_event = mir_input_event_get_keyboard_event(event);
        switch (mir_keyboard_event_action(key_event))
        {
        case mir_keyboard_action_up:
            wl_keyboard_send_key(resource,
                serial,
                mir_input_event_get_event_time(event) / 1000,
                mir_keyboard_event_scan_code(key_event),
                WL_KEYBOARD_KEY_STATE_RELEASED);
            break;
        case mir_keyboard_action_down:
            wl_keyboard_send_key(resource,
                serial,
                mir_input_event_get_event_time(event) / 1000,
                mir_keyboard_event_scan_code(key_event),
                WL_KEYBOARD_KEY_STATE_PRESSED);
        default:
            break;
        }
    }

private:
    virtual void release();
};

void WlKeyboard::release()
{
}

namespace
{
uint32_t calc_button_difference(MirPointerButtons old, MirPointerButtons updated)
{
    switch (old ^ updated)
    {
    case mir_pointer_button_primary:
        return 272; // No, I have *no* idea why GTK expects 271 to be the primary button.
    case mir_pointer_button_secondary:
        return 274;
    case mir_pointer_button_tertiary:
        return 273;
    case mir_pointer_button_back:
        return 275; // I dunno. It's a number, I guess.
    case mir_pointer_button_forward:
        return 276; // I dunno. It's a number, I guess.
    default:
        throw std::logic_error("Whoops, I misunderstand how Mir pointer events work");
    }
}
}

class WlPointer : public wayland::Pointer
{
public:

    WlPointer(wl_client* client, wl_resource* parent, uint32_t id)
        : Pointer(client, parent, id),
          display{wl_client_get_display(client)}
    {
    }

    void handle_event(MirInputEvent const* event, wl_resource* target)
    {
        std::cout << "Hello! In WlPointer::handle_event" << std::endl;
        auto pointer_event = mir_input_event_get_pointer_event(event);

        auto serial = wl_display_next_serial(display);

        switch(mir_pointer_event_action(pointer_event))
        {
        case mir_pointer_action_button_down:
        {
            auto button = calc_button_difference(last_set, mir_pointer_event_buttons(pointer_event));
            wl_pointer_send_button(
                resource,
                serial,
                mir_input_event_get_event_time(event) / 1000,
                button,
                WL_POINTER_BUTTON_STATE_PRESSED);
            last_set = mir_pointer_event_buttons(pointer_event);
            break;
        }
        case mir_pointer_action_button_up:
        {
            auto button = calc_button_difference(last_set, mir_pointer_event_buttons(pointer_event));
            wl_pointer_send_button(
                resource,
                serial,
                mir_input_event_get_event_time(event) / 1000,
                button,
                WL_POINTER_BUTTON_STATE_RELEASED);
            last_set = mir_pointer_event_buttons(pointer_event);
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
    }

    // Pointer interface
private:
    wl_display* const display;
    MirPointerButtons last_set;
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
    delete this;
}

class WlTouch : public wayland::Touch
{
public:
    WlTouch(wl_client* client, wl_resource* parent, uint32_t id)
        : Touch(client, parent, id)
    {
    }

    void handle_event(MirInputEvent const* /*event*/, wl_resource* /*target*/)
    {
        std::cout << "Hello! In WlTouch::handle_event" << std::endl;
    }

    // Touch interface
private:
    virtual void release();
};

void WlTouch::release()
{
}

template<class InputInterface>
class InputCtx
{
public:
    InputCtx() = default;

    InputCtx(InputCtx&&) = delete;
    InputCtx(InputCtx const&) = delete;
    InputCtx& operator=(InputCtx const&) = delete;

    void register_listener(std::shared_ptr<InputInterface> const& listener)
    {
        std::cout << "Registering listener" << std::endl;
        listeners.push_back(listener);
    }

    void unregister_listener(InputInterface const* listener)
    {
        std::remove_if(
            listeners.begin(),
            listeners.end(),
            [listener](auto candidate)
            {
                return candidate.get() == listener;
            });
    }

    void handle_event(MirInputEvent const* event, wl_resource* target) const
    {
        for (auto& listener : listeners)
        {
            std::cout << "Sending event" << std::endl;
            listener->handle_event(event, target);
        }
    }

private:
    std::vector<std::shared_ptr<InputInterface>> listeners;
};

class WlSeat : public wayland::Seat
{
public:
    WlSeat(wl_display* display)
        : Seat(display, 4)
    {
    }

    InputCtx<WlPointer> const& acquire_pointer_reference(wl_client* client) const;
    InputCtx<WlKeyboard> const& acquire_keyboard_reference(wl_client* client) const;
    InputCtx<WlTouch> const& acquire_touch_reference(wl_client* client) const;

private:
    std::unordered_map<wl_client*, InputCtx<WlPointer>> mutable pointer;
    std::unordered_map<wl_client*, InputCtx<WlKeyboard>> mutable keyboard;
    std::unordered_map<wl_client*, InputCtx<WlTouch>> mutable touch;

    void get_pointer(wl_client* client, wl_resource* resource, uint32_t id) override;
    void get_keyboard(wl_client* client, wl_resource* resource, uint32_t id) override;
    void get_touch(wl_client* client, wl_resource* resource, uint32_t id) override;
    void release(struct wl_client* /*client*/, struct wl_resource* /*resource*/) override {}
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

void WlSeat::get_keyboard(wl_client* client, wl_resource* resource, uint32_t id)
{
    keyboard[client].register_listener(std::make_shared<WlKeyboard>(client, resource, id));
}

void WlSeat::get_pointer(wl_client* client, wl_resource* resource, uint32_t id)
{
    pointer[client].register_listener(std::make_shared<WlPointer>(client, resource, id));
}

void WlSeat::get_touch(wl_client* client, wl_resource* resource, uint32_t id)
{
    touch[client].register_listener(std::make_shared<WlTouch>(client, resource, id));
}

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
    void remove_buffer(graphics::Buffer&) override {}
    void update_buffer(graphics::Buffer&) override {}

private:
    std::function<void(MirLifecycleState)> const lifecycle_handler;
};

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

class WlApplication : public wayland::Application
{
public:
    WlApplication(
        std::shared_ptr<mf::Shell> const& shell,
        std::shared_ptr<ClientDepository> const& depository,
        wl_display* display)
        : Application(display, 1),
          shell{shell},
          depository{depository}
    {
    }

private:
    void connect(wl_client* client, wl_resource* resource, const std::string& name) override;

    std::shared_ptr<mf::Shell> const shell;
    std::shared_ptr<ClientDepository> const depository;
};

void WlApplication::connect(wl_client* client, wl_resource* resource, const std::string& name)
{
    pid_t client_pid;
    wl_client_get_credentials(client, &client_pid, nullptr, nullptr);
    auto sink = std::make_shared<WaylandEventSink>(
        [resource](MirLifecycleState state)
        {
            mir_application_send_lifecycle(resource, state);
        });
    depository->add_client_session(
        client,
        shell->open_session(client_pid, name, sink),
        [shell = this->shell](auto session) { shell->close_session(session); });
}

class SurfaceInputSink : public mf::EventSink
{
public:
    SurfaceInputSink(WlSeat* seat, wl_client* client, wl_resource* target)
        : seat{seat},
          client{client},
          target{target}
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
    void remove_buffer(graphics::Buffer&) override {}
    void update_buffer(graphics::Buffer&) override {}

private:
    WlSeat* const seat;
    wl_client* const client;
    wl_resource* const target;
};

void SurfaceInputSink::handle_event(MirEvent const& event)
{
    switch (mir_event_get_type(&event))
    {
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
    }
    default:
        break;
    }
}

class MirSurface
{
public:
    MirSurface(SurfaceId id)
        : id{id}
    {
    }

    SurfaceId const id;
};

class WlWindowSurface : public wayland::WindowSurface, public MirSurface
{
public:
    WlWindowSurface(wl_client *client, wl_resource *parent, uint32_t id, SurfaceId internal_id)
    : WindowSurface(client, parent, id),
      MirSurface(internal_id)
    {
        wl_resource_set_user_data(resource, static_cast<MirSurface*>(this));
    }

private:
    void destroy() override
    {
        delete this;
    }
};

class WlMenuSurface : public wayland::MenuSurface, public MirSurface
{
public:
    WlMenuSurface(wl_client *client, wl_resource *parent, uint32_t id, SurfaceId internal_id)
    : MenuSurface(client, parent, id),
      MirSurface(internal_id)
    {
        wl_resource_set_user_data(resource, static_cast<MirSurface*>(this));
    }

private:
    void destroy() override
    {
        delete this;
    }
};

class WlShell : public wayland::ShellBananas
{
public:
    WlShell(
        std::shared_ptr<mf::Shell> const& shell,
        std::shared_ptr<ClientDepository> const& depository,
        WlSeat* seat,
        wl_display* display)
        : ShellBananas(display, 1),
          shell{shell},
          seat{seat},
          depository{depository}
    {
    }

private:
    std::shared_ptr<mf::Shell> const shell;
    WlSeat* const seat;
    std::shared_ptr<ClientDepository> const depository;

    void as_normal_window(
        wl_client* client,
        wl_resource* resource,
        uint32_t id,
        wl_resource* surface) override;

    void as_freestyle_window(
        wl_client* client,
        wl_resource* resource,
        uint32_t id,
        wl_resource* surface) override;

    void as_menu(
        wl_client* client,
        wl_resource* resource,
        uint32_t id,
        wl_resource* surface,
        wl_resource* parent,
        int32_t top_left_x,
        int32_t top_left_y,
        int32_t bottom_right_x,
        int32_t bottom_right_y,
        int32_t attach_to_edge,
        wl_resource* seat,
        uint32_t serial) override;
};

void WlShell::as_normal_window(
    wl_client* client,
    wl_resource* resource,
    uint32_t id,
    wl_resource* surface)
{
    auto session = depository->session_for_client(client);

    auto surface_ptr = reinterpret_cast<wayland::Surface*>(wl_resource_get_user_data(surface));
    auto my_little_surface = dynamic_cast<WlSurface*>(surface_ptr);

    auto params = ms::SurfaceCreationParameters()
        .of_size(my_little_surface->stream->stream_size())
        .of_buffer_usage(mg::BufferUsage::software)
        .of_pixel_format(mir_pixel_format_argb_8888)
        .of_type(mir_window_type_normal)
        .with_buffer_stream(my_little_surface->stream_id);

    auto const internal_id = shell->create_surface(
        session,
        params,
        std::make_shared<SurfaceInputSink>(seat, client, surface));

    new WlWindowSurface{client, resource, id, internal_id};
}

void WlShell::as_freestyle_window(
    wl_client* client,
    wl_resource* resource,
    uint32_t id,
    wl_resource* surface)
{
    auto session = depository->session_for_client(client);

    auto surface_ptr = reinterpret_cast<wayland::Surface*>(wl_resource_get_user_data(surface));
    auto my_little_surface = dynamic_cast<WlSurface*>(surface_ptr);

    auto params = ms::SurfaceCreationParameters()
        .of_size(my_little_surface->stream->stream_size())
        .of_buffer_usage(mg::BufferUsage::software)
        .of_pixel_format(mir_pixel_format_argb_8888)
        .of_type(mir_window_type_freestyle)
        .with_buffer_stream(my_little_surface->stream_id);

    auto const internal_id = shell->create_surface(
        session,
        params,
        std::make_shared<SurfaceInputSink>(seat, client, surface));

    new WlWindowSurface{client, resource, id, internal_id};
}

void WlShell::as_menu(
    wl_client* client,
    wl_resource* resource,
    uint32_t id,
    wl_resource* surface,
    wl_resource* parent,
    int32_t top_left_x,
    int32_t top_left_y,
    int32_t bottom_right_x,
    int32_t bottom_right_y,
    int32_t attach_to_edge,
    wl_resource* seat,
    uint32_t /*serial*/)
{
    auto session = depository->session_for_client(client);

    auto surface_ptr = reinterpret_cast<wayland::Surface*>(wl_resource_get_user_data(surface));
    auto my_little_surface = dynamic_cast<WlSurface*>(surface_ptr);
    auto parent_internal_id = reinterpret_cast<MirSurface*>(wl_resource_get_user_data(parent))->id;

    auto seat_ptr = reinterpret_cast<wayland::Seat*>(wl_resource_get_user_data(seat));
    auto wl_seat = dynamic_cast<WlSeat*>(seat_ptr);

    MirEdgeAttachment edge;
    switch (attach_to_edge)
    {
    case MIR_SHELL_BANANAS_ATTACHMENT_EDGE_HORIZONTAL:
        edge = mir_edge_attachment_horizontal;
        break;
    case MIR_SHELL_BANANAS_ATTACHMENT_EDGE_VERTICAL:
        edge = mir_edge_attachment_vertical;
        break;
    case MIR_SHELL_BANANAS_ATTACHMENT_EDGE_HORIZONTAL |
         MIR_SHELL_BANANAS_ATTACHMENT_EDGE_VERTICAL:
        edge = mir_edge_attachment_any;
        break;
    }

    geometry::Rectangle the_rect{
        geometry::Point{top_left_x, top_left_y},
        geometry::Size{bottom_right_x - top_left_x, bottom_right_y - top_left_y}};

    auto params = ms::SurfaceCreationParameters()
        .of_size(my_little_surface->stream->stream_size())
        .of_buffer_usage(mg::BufferUsage::software)
        .of_pixel_format(mir_pixel_format_argb_8888)
        .of_type(mir_window_type_menu)
        .with_buffer_stream(my_little_surface->stream_id)
        .with_edge_attachment(edge)
        .with_aux_rect(the_rect)
        .with_parent_id(parent_internal_id);

    auto const internal_id = shell->create_surface(
        session,
        params,
        std::make_shared<SurfaceInputSink>(wl_seat, client, surface));

    new WlMenuSurface{client, resource, id, internal_id};
}


class LifetimeGuard
{
    LifetimeGuard()
        : guard{std::shared_ptr<LifetimeGuard>{this, &on_lifetime_end}}
    {
    }

    ~LifetimeGuard()
    {
        guard.reset();

        std::unique_lock<std::mutex> lock{lifetime_guard};
        {
            if (live)
            {
                liveness_changed.wait(lock, [this]() { return !live; });
            }
        }
    }

    template<typename Guarded>
    std::shared_ptr<Guarded> make_lifetime_guard(Guarded* to_guard)
    {
        return std::shared_ptr<Guarded>(guard, to_guard);
    }

    template<typename Rep, typename Period>
    void wait_for_destruction(std::chrono::duration<Rep, Period> delay)
    {
        guard.reset();

        std::unique_lock<std::mutex> lock{lifetime_guard};
        {
            if (live)
            {
                liveness_changed.wait_for(lock, delay, [this]() { return !live; });
            }
        }
    }

private:
    static void on_lifetime_end(LifetimeGuard* me)
    {
        {
            std::lock_guard<std::mutex> lock{me->lifetime_guard};
            me->live = false;
        }
        me->liveness_changed.notify_all();
    }

    std::mutex lifetime_guard;
    std::condition_variable liveness_changed;
    bool live;

    std::shared_ptr<LifetimeGuard> guard;
};

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
        std::shared_ptr<ms::GlobalEventSender> const& /*hotplug_sink*/,
        mf::DisplayChanger& display_config)
        : display{display}
    {
//        hotplug_sink->subscribe_to_display_events(std::bind(&OutputManager::handle_configuration_change, this, std::placeholders::_1));
//
//        display_config.active_configuration()->for_each_output(std::bind(&OutputManager::create_output, this, std::placeholders::_1));
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
    std::shared_ptr<mf::Shell> const& shell,
    std::shared_ptr<mf::EventSink> const& global_sink,
    mf::DisplayChanger& display_config)
    : display{wl_display_create(), &cleanup_display},
      pause_signal{eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE)}
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

    auto depository = std::make_shared<ClientDepository>();

    seat_global = std::make_unique<mf::WlSeat>(display.get());
    application_global = std::make_unique<mf::WlApplication>(shell, depository, display.get());
    compositor_global = std::make_unique<mf::WlCompositor>(depository, display.get());
    shell_global = std::make_unique<mf::WlShell>(shell, depository, seat_global.get(), display.get());
    output_manager = std::make_unique<mf::OutputManager>(
        display.get(),
        std::dynamic_pointer_cast<ms::GlobalEventSender>(global_sink),
        display_config);

    wl_display_init_shm(display.get());

    wl_display_add_socket_auto(display.get());

    auto wayland_loop = wl_display_get_event_loop(display.get());

    wl_event_loop_add_fd(wayland_loop, pause_signal, WL_EVENT_READABLE, &halt_eventloop, display.get());
}

mf::WaylandConnector::~WaylandConnector()
{
    if (dispatch_thread.joinable())
    {
        stop();
    }
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
    dispatch_thread.join();
    dispatch_thread = std::thread{};
}

int mf::WaylandConnector::client_socket_fd() const
{
    return -1;
}

int mf::WaylandConnector::client_socket_fd(
    std::function<void(std::shared_ptr<Session> const& session)> const& /*connect_handler*/) const
{
    return -1;
}
