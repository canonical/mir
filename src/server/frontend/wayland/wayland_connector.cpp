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

#include "wayland_utils.h"
#include "wl_mir_window.h"
#include "wl_surface.h"
#include "wl_seat.h"

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

#include "mir/input/seat.h"
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
#include <unordered_set>
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
}

std::shared_ptr<mir::frontend::Session> get_session(wl_client* client)
{
    auto listener = wl_client_get_destroy_listener(client, &cleanup_private);

    if (listener)
        return private_from_listener(listener)->session;

    return nullptr;
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
        auto* const mir_surface = WlSurface::from(surface);
        mir_surface->set_role(null_wl_mir_window_ptr);
    }

protected:
    void destroy() override
    {
        wl_resource_destroy(resource);
    }

    void set_toplevel() override
    {
        auto* const mir_surface = WlSurface::from(surface);

        mir_surface->set_role(this);
    }

    void set_transient(
        struct wl_resource* parent,
        int32_t x,
        int32_t y,
        uint32_t flags) override
    {
        auto const session = get_session(client);
        auto& parent_surface = *WlSurface::from(parent);

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
                params->type = mir_window_type_gloss;
            params->parent_id = parent_surface.surface_id;
            params->aux_rect = geom::Rectangle{{x, y}, {}};
            params->surface_placement_gravity = mir_placement_gravity_northwest;
            params->aux_rect_placement_gravity = mir_placement_gravity_southeast;
            params->placement_hints = mir_placement_hints_slide_x;
            params->aux_rect_placement_offset_x = 0;
            params->aux_rect_placement_offset_y = 0;

            auto* const mir_surface = WlSurface::from(surface);
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
            params->state = mir_window_state_fullscreen;
            if (output)
            {
                // TODO{alan_g} params->output_id = DisplayConfigurationOutputId_from(output)
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
        auto const session = get_session(client);
        auto& parent_surface = *WlSurface::from(parent);

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
                params->type = mir_window_type_gloss;

            params->parent_id = parent_surface.surface_id;
            params->aux_rect = geom::Rectangle{{x, y}, {}};
            params->surface_placement_gravity = mir_placement_gravity_northwest;
            params->aux_rect_placement_gravity = mir_placement_gravity_southeast;
            params->placement_hints = mir_placement_hints_slide_x;
            params->aux_rect_placement_offset_x = 0;
            params->aux_rect_placement_offset_y = 0;

            auto* const mir_surface = WlSurface::from(surface);
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
            params->state = mir_window_state_maximized;
            if (output)
            {
                // TODO{alan_g} params->output_id = DisplayConfigurationOutputId_from(output)
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
            params->name = title;
        }
    }

    void pong(uint32_t /*serial*/) override
    {
    }

    void move(struct wl_resource* /*seat*/, uint32_t /*serial*/) override
    {
        if (surface_id.as_value())
        {
            if (auto session = get_session(client))
            {
                shell->request_operation(session, surface_id, sink->latest_timestamp(), Shell::UserRequest::move);
            }
        }
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

    void move(struct wl_resource* seat, uint32_t serial) override;

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
        auto* const mir_surface = WlSurface::from(surface);
        mir_surface->set_role(null_wl_mir_window_ptr);
    }

    void destroy() override
    {
        wl_resource_destroy(resource);
    }

    void get_toplevel(uint32_t id) override
    {
        new XdgToplevelV6{client, parent, id, shell, this};
        auto* const mir_surface = WlSurface::from(surface);
        mir_surface->set_role(this);
    }

    void get_popup(uint32_t id, struct wl_resource* parent, struct wl_resource* positioner) override
    {
        auto* tmp = wl_resource_get_user_data(positioner);
        auto const* const pos =  static_cast<XdgPositionerV6*>(static_cast<wayland::XdgPositionerV6*>(tmp));

        auto const session = get_session(client);
        auto& parent_surface = *get_xdgsurface(parent);

        params->type = mir_window_type_freestyle;
        params->parent_id = parent_surface.surface_id;
        if (pos->size.is_set()) params->size = pos->size.value();
        params->aux_rect = pos->aux_rect;
        params->surface_placement_gravity = pos->surface_placement_gravity;
        params->aux_rect_placement_gravity = pos->aux_rect_placement_gravity;
        params->aux_rect_placement_offset_x = pos->aux_rect_placement_offset_x;
        params->aux_rect_placement_offset_y = pos->aux_rect_placement_offset_y;
        params->placement_hints = mir_placement_hints_slide_any;

        new XdgPopupV6{client, parent, id};
        auto* const mir_surface = WlSurface::from(surface);
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
            params->input_shape = {input_region};
        }
    }

    void ack_configure(uint32_t serial) override
    {
        (void)serial;
        // TODO
    }

    void set_parent(optional_value<SurfaceId> parent_id);
    void set_title(std::string const& title);
    void move(struct wl_resource* seat, uint32_t serial);
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
        params->name = title;
    }
}

void XdgSurfaceV6::move(struct wl_resource* /*seat*/, uint32_t /*serial*/)
{
    if (surface_id.as_value())
    {
        if (auto session = get_session(client))
        {
            shell->request_operation(session, surface_id, sink->latest_timestamp(), Shell::UserRequest::move);
        }
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
        params->parent_id = parent_id;
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
            if (params->max_width.is_set())
                params->max_width.consume();
        }
        else
            params->max_width = geom::Width{width};

        if (height == 0)
        {
            if (params->max_height.is_set())
                params->max_height.consume();
        }
        else
            params->max_height = geom::Height{height};
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
        params->min_width = geom::Width{width};
        params->min_height = geom::Height{height};
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
        params->state = mir_window_state_maximized;
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
        params->state = mir_window_state_restored;
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

void XdgToplevelV6::move(struct wl_resource* seat, uint32_t serial)
{
    self->move(seat, serial);
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

class ThrowingAllocator : public mg::WaylandAllocator
{
public:
    void bind_display(wl_display*) override
    {
    }

    std::shared_ptr<mir::graphics::Buffer> buffer_from_resource(
        wl_resource*,
        std::function<void()>&&,
        std::function<void()>&&) override
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"buffer_from_resource called on invalid allocator."}));
    }
};

std::shared_ptr<mg::WaylandAllocator> allocator_for_display(
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    wl_display* display)
{
    if (auto allocator = std::dynamic_pointer_cast<mg::WaylandAllocator>(buffer_allocator))
    {
        try
        {
            allocator->bind_display(display);
            return allocator;
        }
        catch (...)
        {
            mir::log(
                mir::logging::Severity::warning,
                "Wayland",
                std::current_exception(),
                "Failed to bind Wayland EGL display. Accelerated EGL will be unavailable.");
        }
    }
    /*
     * If we don't have a WaylandAllocator or it failed to bind to the display then there *should*
     * be no way for a client to send a non-shm buffer.
     *
     * In case a client manages to do something stupid return a valid allocator that will
     * just throw an exception (and disconnect the client) if something somehow tries to
     * import a buffer.
     */
    return std::make_shared<ThrowingAllocator>();
}
}

mf::WaylandConnector::WaylandConnector(
    optional_value<std::string> const& display_name,
    std::shared_ptr<mf::Shell> const& shell,
    DisplayChanger& display_config,
    std::shared_ptr<mi::InputDeviceHub> const& input_hub,
    std::shared_ptr<mi::Seat> const& seat,
    std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<mf::SessionAuthorizer> const& session_authorizer,
    bool arw_socket)
    : display{wl_display_create(), &cleanup_display},
      pause_signal{eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE)},
      allocator{allocator_for_display(allocator, display.get())}
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
    seat_global = std::make_unique<mf::WlSeat>(display.get(), input_hub, seat, executor);
    output_manager = std::make_unique<mf::OutputManager>(
        display.get(),
        display_config);
    shell_global = std::make_unique<mf::WlShell>(display.get(), shell, *seat_global);
    data_device_manager_global = std::make_unique<DataDeviceManager>(display.get());

    // The XDG shell support is currently too flaky to enable by default
    if (getenv("MIR_EXPERIMENTAL_XDG_SHELL"))
        xdg_shell_global = std::make_unique<XdgShellV6>(display.get(), shell, *seat_global);

    wl_display_init_shm(display.get());

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
