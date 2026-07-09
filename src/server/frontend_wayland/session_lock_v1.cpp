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

#include "client.h"
#include "output_manager.h"
#include "protocol_error.h"
#include "weak.h"
#include "wl_surface.h"
#include "window_wl_surface_role.h"

#include <mir/shell/surface_specification.h>
#include <mir/scene/session_lock.h>
#include <mir/log.h>

#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace mw = mir::wayland_rs;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace ms = mir::scene;

namespace mir
{
namespace frontend
{
class SessionLockManagerV1 : public mw::ExtSessionLockManagerV1
{
public:
    SessionLockManagerV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::ExtSessionLockManagerV1Middleware> instance,
        uint32_t object_id,
        Executor& wayland_executor,
        std::shared_ptr<msh::Shell> const& shell,
        std::shared_ptr<ms::SessionLock> const& session_lock,
        WlSeat& seat,
        OutputManager* output_manager,
        std::shared_ptr<SurfaceRegistry> const& surface_registry,
        std::shared_ptr<SessionLockState> const& state)
        : ExtSessionLockManagerV1{std::move(client), std::move(instance), object_id},
          wayland_executor{wayland_executor},
          shell{shell},
          session_lock{session_lock},
          seat{seat},
          output_manager{output_manager},
          surface_registry{surface_registry},
          state{state}
    {
    }

    Executor& wayland_executor;
    std::shared_ptr<msh::Shell> const shell;
    std::shared_ptr<ms::SessionLock> const session_lock;
    WlSeat& seat;
    OutputManager* const output_manager;
    std::shared_ptr<SurfaceRegistry> const surface_registry;
    std::shared_ptr<SessionLockState> const state;

private:
    auto lock(
        rust::Box<mw::ExtSessionLockV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::ExtSessionLockV1> override;
};

class SessionLockV1 : public mw::ExtSessionLockV1
{
public:
    SessionLockV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::ExtSessionLockV1Middleware> instance,
        uint32_t object_id,
        SessionLockManagerV1& manager);
    ~SessionLockV1();

private:
    class SessionLockV1Observer : public mir::scene::SessionLockObserver
    {
    public:
        explicit SessionLockV1Observer(SessionLockV1& lock);
        void on_lock() override;
        void on_unlock() override;

    private:
        SessionLockV1& lock;
    };

    auto get_lock_surface(
        mw::Weak<mw::Surface> const& surface,
        mw::Weak<mw::Output> const& output,
        rust::Box<mw::ExtSessionLockSurfaceV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::ExtSessionLockSurfaceV1> override;
    void destroy() override;
    void unlock_and_destroy() override;

    SessionLockManagerV1& manager;
    /* Lifetime annotation:
     * This is safe even though `SessionLockV1Observer` keeps a reference to `this`
     * because the `shared_ptr` is only only shared with the `ObserverRegistrar<>`
     * (which uses a `weak_ptr` internally) and observations are delegated to the
     * `WaylandExecutor` so all interactions are single-threaded
     */
    std::shared_ptr<SessionLockV1Observer> const observer;
};

class SessionLockSurfaceV1 : public mw::ExtSessionLockSurfaceV1, public WindowWlSurfaceRole
{
public:
    using mw::ExtSessionLockSurfaceV1::destroyed_flag;

    SessionLockSurfaceV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::ExtSessionLockSurfaceV1Middleware> instance,
        uint32_t object_id,
        SessionLockManagerV1& manager,
        WlSurface* surface,
        graphics::DisplayConfigurationOutputId output_id);
    ~SessionLockSurfaceV1() = default;

private:
    // From wayland_rs::ExtSessionLockSurfaceV1
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
        const_cast<SessionLockSurfaceV1*>(this)->destroy_and_delete();
    }
};

}
}

// SessionLockState

mf::SessionLockState::SessionLockState(std::shared_ptr<ms::SessionLock> session_lock)
    : session_lock{std::move(session_lock)}
{
}

bool mf::SessionLockState::try_lock(SessionLockV1* lock)
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

bool mf::SessionLockState::try_relinquish_locking_privilege(SessionLockV1* lock)
{
    if (active_lock == lock)
    {
        active_lock = nullptr;
        return true;
    }

    return false;
}

bool mf::SessionLockState::try_unlock(SessionLockV1* lock)
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

bool mf::SessionLockState::is_active_lock(SessionLockV1* lock) const
{
    return lock == active_lock;
}

// create_session_lock_manager_v1

auto mf::create_session_lock_manager_v1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::ExtSessionLockManagerV1Middleware> instance,
    uint32_t object_id,
    Executor& wayland_executor,
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<ms::SessionLock> const& session_lock,
    WlSeat& seat,
    OutputManager* output_manager,
    std::shared_ptr<SurfaceRegistry> const& surface_registry,
    std::shared_ptr<SessionLockState> const& state)
    -> std::shared_ptr<mw::ExtSessionLockManagerV1>
{
    return std::make_shared<SessionLockManagerV1>(
        std::move(client),
        std::move(instance),
        object_id,
        wayland_executor,
        shell,
        session_lock,
        seat,
        output_manager,
        surface_registry,
        state);
}

// SessionLockManagerV1

auto mf::SessionLockManagerV1::lock(
    rust::Box<mw::ExtSessionLockV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mw::ExtSessionLockV1>
{
    return std::make_shared<SessionLockV1>(client, std::move(child_instance), child_object_id, *this);
}

// SessionLockV1

mf::SessionLockV1::SessionLockV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::ExtSessionLockV1Middleware> instance,
    uint32_t object_id,
    SessionLockManagerV1& manager)
    : ExtSessionLockV1{std::move(client), std::move(instance), object_id},
      manager{manager},
      observer{std::make_shared<SessionLockV1Observer>(*this)}
{
    if (manager.state->try_lock(this))
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
    manager.state->try_relinquish_locking_privilege(this);
}

auto mf::SessionLockV1::get_lock_surface(
    mw::Weak<mw::Surface> const& surface,
    mw::Weak<mw::Output> const& output,
    rust::Box<mw::ExtSessionLockSurfaceV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mw::ExtSessionLockSurfaceV1>
{
    auto* const output_ptr = mw::as_nullable_ptr(output);
    auto const output_id = OutputManager::output_id_for(output_ptr);
    if (!output_id)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "wl_output@" +
            std::to_string(output_ptr ? output_ptr->object_id() : 0) +
            " does not map to a valid Mir output ID"));
    }

    return std::make_shared<SessionLockSurfaceV1>(
        client,
        std::move(child_instance),
        child_object_id,
        manager,
        mw::Surface::from<WlSurface>(surface),
        output_id.value());
}

void mf::SessionLockV1::destroy()
{
    if (manager.state->is_active_lock(this))
    {
        manager.state->try_relinquish_locking_privilege(this);
        mir::log_debug("SessionLockV1 successfully destroyed itself");
    }
    else
    {
        throw mw::ProtocolError{
            object_id(),
            Error::invalid_destroy,
            "Destroy requested but session is still locked"};
    }
}

void mf::SessionLockV1::unlock_and_destroy()
{
    if (!manager.state->try_unlock(this))
    {
        throw mw::ProtocolError{
            object_id(),
            Error::invalid_unlock,
            "Unlock requested but locked event was never sent"};
    }
    else
    {
        manager.state->try_relinquish_locking_privilege(this);
        mir::log_debug("SessionLockV1 successfully unlocked_and_destroyed itself");
    }
}

mf::SessionLockV1::SessionLockV1Observer::SessionLockV1Observer(SessionLockV1& lock)
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
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::ExtSessionLockSurfaceV1Middleware> instance,
    uint32_t object_id,
    SessionLockManagerV1& manager,
    WlSurface* surface,
    graphics::DisplayConfigurationOutputId output_id)
    : ExtSessionLockSurfaceV1{std::move(client), std::move(instance), object_id},
      WindowWlSurfaceRole(
          manager.wayland_executor,
          &manager.seat,
          mw::ExtSessionLockSurfaceV1::client,
          surface,
          manager.shell,
          manager.output_manager,
          manager.surface_registry)
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
    auto const serial = mw::ExtSessionLockSurfaceV1::client->next_serial(nullptr);
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
    auto const serial = mw::ExtSessionLockSurfaceV1::client->next_serial(nullptr);
    send_configure_event(serial, new_size.width.as_int(), new_size.height.as_int());
}
