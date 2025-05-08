/*
 * Copyright © Canonical Ltd.
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
 */

#include "session_lock_v1.h"

#include "wl_surface.h"
#include "window_wl_surface_role.h"
#include "output_manager.h"

#include "mir/shell/surface_specification.h"
#include "mir/fatal.h"
#include "mir/wayland/client.h"
#include "mir/wayland/protocol_error.h"
#include "mir/scene/session_lock.h"
#include "mir/frontend/surface_stack.h"
#include "mir/log.h"

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mw = mir::wayland;
namespace ms = mir::scene;

namespace mir
{
namespace frontend
{
class SessionLockManagerV1::Instance : wayland::SessionLockManagerV1
{
public:
    Instance(wl_resource* new_resource, mf::SessionLockManagerV1& manager)
        : SessionLockManagerV1{new_resource, Version<1>()},
          manager{manager}
    {
    }

private:
    void lock(wl_resource* lock) override;
    mf::SessionLockManagerV1& manager;
};

class SessionLockV1 : wayland::SessionLockV1
{
public:
    SessionLockV1(wl_resource* new_resource, mf::SessionLockManagerV1& manager);
    ~SessionLockV1();

private:
    class SessionLockV1Observer : public mir::scene::SessionLockObserver
    {
    public:
        explicit SessionLockV1Observer (mf::SessionLockV1& lock);
        void on_lock() override;
        void on_unlock() override;

    private:
        SessionLockV1& lock;
    };

    void get_lock_surface(wl_resource* id, wl_resource* surface, wl_resource* output) override;
    void destroy() override;
    void unlock_and_destroy() override;

    mf::SessionLockManagerV1& manager;
    /* Lifetime annotation:
     * This is safe even though `SessionLockV1Observer` keeps a reference to `this`
     * because the `shared_ptr` is only only shared with the `ObserverRegistrar<>`
     * (which uses a `weak_ptr` internally) and observations are delegated to the
     * `WaylandExecutor` so all interactions are single-threaded
     */
    std::shared_ptr<SessionLockV1Observer> const observer;
};

class SessionLockSurfaceV1 : wayland::SessionLockSurfaceV1, public WindowWlSurfaceRole
{
public:
    SessionLockSurfaceV1(
        wl_resource* new_resource,
        mf::SessionLockManagerV1& manager,
        WlSurface* surface,
        graphics::DisplayConfigurationOutputId output_id);
    ~SessionLockSurfaceV1() = default;

private:
    // From wayland::SessionLockSurfaceV1
    void ack_configure(uint32_t serial) override;

    // From WindowWlSurfaceRole
    void handle_commit() override {};
    void handle_state_change(MirWindowState /*new_state*/) override {};
    void handle_active_change(bool /*is_now_active*/) override {};
    void handle_resize(std::optional<geometry::Point> const&, geometry::Size const& new_size) override;
    void handle_close_request() override {};
    void handle_tiled_edges(Flags<MirTiledEdge> /*tiled_edges*/) override {}

    void destroy_role() const override
    {
        wl_resource_destroy(resource);
    }
};

}
}

// SessionLockManagerV1

mf::SessionLockManagerV1::SessionLockManagerV1(
    struct wl_display* display,
    Executor& wayland_executor,
    std::shared_ptr<msh::Shell> shell,
    std::shared_ptr<ms::SessionLock> session_lock,
    WlSeat& seat,
    OutputManager* output_manager,
    std::shared_ptr<SurfaceStack> surface_stack)
    : Global(display, Version<1>()),
      wayland_executor{wayland_executor},
      shell{std::move(shell)},
      session_lock{std::move(session_lock)},
      seat{seat},
      output_manager{output_manager},
      surface_stack{surface_stack}
{
}

bool mf::SessionLockManagerV1::try_lock(SessionLockV1* lock)
{
    if (active_lock == nullptr)
    {
        mir::log_debug("SessionLockManagerV1 has successfully processed a lock");
        active_lock = lock;
        session_lock->lock();
        return true;
    }

    mir::log_debug("SessionLockManagerV1 has failed to process a lock");
    return false;
}

bool mf::SessionLockManagerV1::try_relinquish_locking_privilege(mir::frontend::SessionLockV1 *lock)
{
    if (active_lock == lock)
    {
        active_lock = nullptr;
        return true;
    }

    return false;
}

bool mf::SessionLockManagerV1::try_unlock(SessionLockV1* lock)
{
    if (try_relinquish_locking_privilege(lock))
    {
        mir::log_debug("SessionLockManagerV1 has successfully processed an unlock");
        session_lock->unlock();
        return true;
    }

    mir::log_debug("SessionLockManagerV1 has failed to process an unlock");
    return false;
}

bool mf::SessionLockManagerV1::is_active_lock(SessionLockV1* lock)
{
    return lock == active_lock;
}

void mf::SessionLockManagerV1::bind(wl_resource* new_resource)
{
    new Instance{new_resource, *this};
}

// SessionLockManagerV1::Instance

void mf::SessionLockManagerV1::Instance::lock(wl_resource* lock)
{
    new SessionLockV1(lock, manager);
}

// SessionLockV1

mf::SessionLockV1::SessionLockV1(wl_resource* new_resource, SessionLockManagerV1& manager)
    : mw::SessionLockV1(new_resource, Version<1>()),
      manager{manager},
      observer{std::make_shared<SessionLockV1Observer>(*this)}
{
    if (manager.try_lock(this))
    {
        mir::log_debug("SessionLockV1 successfully acquired the lock");
        send_locked_event();
        manager.session_lock->register_interest(observer, manager.wayland_executor);
    }
    else
    {
        mir::log_debug("SessionLockV1 failed to acquire the lock");
        send_finished_event();
    }
}

mf::SessionLockV1::~SessionLockV1()
{
    mir::log_debug("SessionLockV1 is dying");
    manager.try_relinquish_locking_privilege(this);
}

void mf::SessionLockV1::get_lock_surface(wl_resource* id, wl_resource* surface_resource, wl_resource* output)
{
    auto const output_id = OutputManager::output_id_for(output);
    if (!output_id)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "wl_output@" +
            std::to_string(wl_resource_get_id(output)) +
            " does not map to a valid Mir output ID"));
    }

    new SessionLockSurfaceV1{id, manager, WlSurface::from(surface_resource), output_id.value()};
}

void mf::SessionLockV1::destroy()
{
    if (manager.is_active_lock(this))
    {
        manager.try_relinquish_locking_privilege(this);
        mir::log_debug("SessionLockV1 successfully destroyed itself");
    }
    else
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            Error::invalid_destroy,
            "Destroy requested but session is still locked"));
    }
}

void mf::SessionLockV1::unlock_and_destroy()
{
    if (!manager.try_unlock(this))
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            Error::invalid_unlock,
            "Unlock requested but locked event was never sent"));
    }
    else
    {
        manager.try_relinquish_locking_privilege(this);
        mir::log_debug("SessionLockV1 successfully unlocked_and_destroyed itself");
    }
}

mf::SessionLockV1::SessionLockV1Observer::SessionLockV1Observer(mf::SessionLockV1& lock)
    : lock{lock}
{
}

void mf::SessionLockV1::SessionLockV1Observer::on_lock() {}

void mf::SessionLockV1::SessionLockV1Observer::on_unlock()
{
    lock.send_finished_event();
}

// SessionLockSurfaceV1

mf::SessionLockSurfaceV1::SessionLockSurfaceV1(
    wl_resource* new_resource,
    mf::SessionLockManagerV1& manager,
    WlSurface* surface,
    graphics::DisplayConfigurationOutputId output_id)
    : mw::SessionLockSurfaceV1(new_resource, Version<1>()),
      WindowWlSurfaceRole(
          manager.wayland_executor,
          &manager.seat,
          mw::SessionLockSurfaceV1::client,
          surface,
          manager.shell,
          manager.output_manager)
{
    shell::SurfaceSpecification spec;
    spec.state = mir_window_state_attached;
    spec.depth_layer = mir_depth_layer_overlay;
    spec.focus_mode = mir_focus_mode_grabbing;
    spec.output_id = output_id;
    spec.attached_edges = MirPlacementGravity(mir_placement_gravity_northwest | mir_placement_gravity_southeast);
    spec.visible_on_lock_screen = true;
    spec.ignore_exclusion_zones = true;
    apply_spec(spec);
    auto const serial = Resource::client->next_serial(nullptr);
    send_configure_event(serial, 100, 100);
}

void mf::SessionLockSurfaceV1::ack_configure(uint32_t serial)
{
    (void)serial;
}

void mf::SessionLockSurfaceV1::handle_resize(std::optional<geom::Point> const&, geom::Size const& new_size)
{
    set_pending_width(new_size.width);
    set_pending_height(new_size.height);
    auto const serial = Resource::client->next_serial(nullptr);
    send_configure_event(serial, new_size.width.as_int(), new_size.height.as_int());
}
