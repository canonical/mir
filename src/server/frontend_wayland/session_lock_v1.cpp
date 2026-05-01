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
#include "client.h"
#include "protocol_error.h"

#include <mir/shell/surface_specification.h>
#include <mir/fatal.h>
#include <mir/scene/session_lock.h>
#include <mir/frontend/surface_stack.h>
#include <mir/log.h>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mw = mir::wayland_rs;
namespace ms = mir::scene;

namespace mir
{
namespace frontend
{
class SessionLockManagerV1::Instance : public mw::ExtSessionLockManagerV1Impl
{
public:
    Instance(
        std::shared_ptr<wayland_rs::Client> const& client,
        mf::SessionLockManagerV1& manager,
        std::shared_ptr<SurfaceRegistry> const& surface_registry)
        : client{client},
          manager{manager},
          surface_registry{surface_registry}
    {
    }

    auto lock() -> std::shared_ptr<wayland_rs::ExtSessionLockV1Impl> override;

private:
    std::shared_ptr<wayland_rs::Client> client;
    mf::SessionLockManagerV1& manager;
    std::shared_ptr<SurfaceRegistry> const surface_registry;
};

class SessionLockV1 : public mw::ExtSessionLockV1Impl
{
public:
    SessionLockV1(std::shared_ptr<wayland_rs::Client> const& client, mf::SessionLockManagerV1& manager);
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

    auto get_lock_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface, wayland_rs::Weak<wayland_rs::WlOutputImpl> const& output) -> std::shared_ptr<wayland_rs::ExtSessionLockSurfaceV1Impl> override;
    void destroy() override;
    void unlock_and_destroy() override;

    std::shared_ptr<wayland_rs::Client> client;
    mf::SessionLockManagerV1& manager;
    /* Lifetime annotation:
     * This is safe even though `SessionLockV1Observer` keeps a reference to `this`
     * because the `shared_ptr` is only only shared with the `ObserverRegistrar<>`
     * (which uses a `weak_ptr` internally) and observations are delegated to the
     * `WaylandExecutor` so all interactions are single-threaded
     */
    std::shared_ptr<SessionLockV1Observer> const observer;
};

class SessionLockSurfaceV1 : public mw::ExtSessionLockSurfaceV1Impl, public WindowWlSurfaceRole
{
public:
    SessionLockSurfaceV1(
        std::shared_ptr<wayland_rs::Client> const& client,
        mf::SessionLockManagerV1& manager,
        WlSurface* surface,
        graphics::DisplayConfigurationOutputId output_id);
    ~SessionLockSurfaceV1() = default;

    auto associate(rust::Box<mw::ExtSessionLockSurfaceV1Ext> instance, uint32_t object_id) -> void override;

    // From mw::SessionLockSurfaceV1
    void ack_configure(uint32_t serial) override;
private:

    // From WindowWlSurfaceRole
    void handle_commit() override {};
    void handle_state_change(MirWindowState /*new_state*/) override {};
    void handle_active_change(bool /*is_now_active*/) override {};
    void handle_resize(std::optional<geometry::Point> const&, geometry::Size const& new_size) override;
    void handle_close_request() override {};
    void handle_tiled_edges(Flags<MirTiledEdge> /*tiled_edges*/) override {}

    void destroy_role() const override
    {
        // TODO:
        // wl_resource_destroy(resource);
    }

    std::shared_ptr<wayland_rs::Client> client;
};

}
}

// SessionLockManagerV1

mf::SessionLockManagerV1::SessionLockManagerV1(
    Executor& wayland_executor,
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<ms::SessionLock> const& session_lock,
    WlSeatGlobal& seat,
    OutputManager* output_manager,
    std::shared_ptr<SurfaceStack> const& surface_stack,
    std::shared_ptr<SurfaceRegistry> const& surface_registry)
    : wayland_executor{wayland_executor},
      shell{std::move(shell)},
      session_lock{std::move(session_lock)},
      seat{seat},
      output_manager{output_manager},
      surface_stack{surface_stack},
      surface_registry{surface_registry}
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

auto mir::frontend::SessionLockManagerV1::create(std::shared_ptr<wayland_rs::Client> const& client) -> std::shared_ptr<wayland_rs::ExtSessionLockManagerV1Impl>
{
    return std::make_shared<Instance>(client, *this, surface_registry);
}

// SessionLockManagerV1::Instance
auto mf::SessionLockManagerV1::Instance::lock() -> std::shared_ptr<wayland_rs::ExtSessionLockV1Impl>
{
    return std::make_shared<SessionLockV1>(client, manager);
}

// SessionLockV1

mf::SessionLockV1::SessionLockV1(std::shared_ptr<wayland_rs::Client> const& client, SessionLockManagerV1& manager)
    : client{client},
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

auto mir::frontend::SessionLockV1::get_lock_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface, wayland_rs::Weak<wayland_rs::WlOutputImpl> const& output) -> std::shared_ptr<wayland_rs::ExtSessionLockSurfaceV1Impl>
{
    auto const output_id = OutputManager::output_id_for(&output.value());
    if (!output_id)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "wl_output@" +
            std::to_string(object_id()) +
            " does not map to a valid Mir output ID"));
    }

    return std::make_shared<SessionLockSurfaceV1>(client, manager, WlSurface::from(&surface.value()), output_id.value());
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
            object_id(),
            Error::invalid_destroy,
            "Destroy requested but session is still locked"));
    }
}

void mf::SessionLockV1::unlock_and_destroy()
{
    if (!manager.try_unlock(this))
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
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
    std::shared_ptr<wayland_rs::Client> const& client,
    mf::SessionLockManagerV1& manager,
    WlSurface* surface,
    graphics::DisplayConfigurationOutputId output_id)
    : WindowWlSurfaceRole(
          manager.wayland_executor,
          &manager.seat,
          client.get(),
          surface->shared_from_this(),
          manager.shell,
          manager.output_manager,
          manager.surface_registry),
      client{client}
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
}

auto mf::SessionLockSurfaceV1::associate(rust::Box<mw::ExtSessionLockSurfaceV1Ext> instance, uint32_t object_id) -> void
{
    ExtSessionLockSurfaceV1Impl::associate(std::move(instance), object_id);
    init_observer();
    auto const serial = client->next_serial(nullptr);
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
    auto const serial = client->next_serial(nullptr);
    send_configure_event(serial, new_size.width.as_int(), new_size.height.as_int());
}
