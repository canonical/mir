/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "zwp_pointer_constraints_v1.h"
#include "wl_region.h"
#include "wl_surface.h"

#include <mir/executor.h>
#include <mir/scene/surface.h>
#include <mir/scene/null_surface_observer.h>
#include <mir/shell/shell.h>
#include <mir/shell/surface_specification.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace ms = mir::scene;

namespace
{
enum class ConstraintLifetime : uint32_t
{
    oneshot = mw::PointerConstraintsV1::Lifetime::oneshot,
    persistent = mw::PointerConstraintsV1::Lifetime::persistent,
};

class LockedPointerV1 : public mw::LockedPointerV1
{
public:
    LockedPointerV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::LockedPointerV1Middleware> instance,
        uint32_t object_id,
        mir::Executor& wayland_executor,
        std::shared_ptr<mir::shell::Shell> shell,
        std::shared_ptr<ms::Surface> const& scene_surface,
        mf::WlRegion* region,
        ConstraintLifetime lifetime);
    ~LockedPointerV1();

private:
    std::shared_ptr<mir::shell::Shell> const shell;
    std::weak_ptr<ms::Surface> const weak_scene_surface;

    struct MyWaylandSurfaceObserver;

    std::shared_ptr<MyWaylandSurfaceObserver> const my_surface_observer;

    void set_cursor_position_hint(double /*surface_x*/, double /*surface_y*/) override;
    void set_region(std::optional<mw::Weak<mw::Region>> const& /*region*/) override;
};

class ConfinedPointerV1 : public mw::ConfinedPointerV1
{
public:
    ConfinedPointerV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::ConfinedPointerV1Middleware> instance,
        uint32_t object_id,
        mir::Executor& wayland_executor,
        std::shared_ptr<mir::shell::Shell> shell,
        std::shared_ptr<ms::Surface> const& scene_surface,
        mf::WlRegion* region,
        ConstraintLifetime lifetime);
    ~ConfinedPointerV1();

private:
    std::shared_ptr<mir::shell::Shell> const shell;
    std::weak_ptr<ms::Surface> const weak_scene_surface;

    struct SurfaceObserver;

    std::shared_ptr<SurfaceObserver> const my_surface_observer;

    void set_region(std::optional<mw::Weak<mw::Region>> const& /*region*/) override;
};

struct LockedPointerV1::MyWaylandSurfaceObserver : ms::NullSurfaceObserver
{
    MyWaylandSurfaceObserver(LockedPointerV1* const self) : self{self} {}

    void attrib_changed(const ms::Surface* surf, MirWindowAttrib attrib, int value) override;
    mw::Weak<LockedPointerV1> const self;
};

void LockedPointerV1::MyWaylandSurfaceObserver::attrib_changed(
    ms::Surface const* surf,
    MirWindowAttrib attrib,
    int value)
{
    if (attrib == mir_window_attrib_focus)
    {
        switch (surf->confine_pointer_state())
        {
        case mir_pointer_locked_persistent:
        case mir_pointer_locked_oneshot:
            if (self)
            {
                if (value == mir_window_focus_state_unfocused)
                {
                    self.value().send_unlocked_event();
                }
                else
                {
                    self.value().send_locked_event();
                }
            }
            break;

        default:
            break;
        }
    }
    NullSurfaceObserver::attrib_changed(surf, attrib, value);
}

struct ConfinedPointerV1::SurfaceObserver : ms::NullSurfaceObserver
{
    SurfaceObserver(ConfinedPointerV1* const self) : self{self} {}

    void attrib_changed(const ms::Surface* surf, MirWindowAttrib attrib, int value) override;
    mw::Weak<ConfinedPointerV1> const self;
};

void ConfinedPointerV1::SurfaceObserver::attrib_changed(
    ms::Surface const* surf,
    MirWindowAttrib attrib,
    int value)
{
    if (attrib == mir_window_attrib_focus)
    {
        switch (surf->confine_pointer_state())
        {
        case mir_pointer_confined_persistent:
        case mir_pointer_confined_oneshot:
            if (self)
            {
                if (value == mir_window_focus_state_unfocused)
                {
                    self.value().send_unconfined_event();
                }
                else
                {
                    self.value().send_confined_event();
                }
            }
            break;

        default:
            break;
        }
    }
    NullSurfaceObserver::attrib_changed(surf, attrib, value);
}

LockedPointerV1::LockedPointerV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::LockedPointerV1Middleware> instance,
    uint32_t object_id,
    mir::Executor& wayland_executor,
    std::shared_ptr<mir::shell::Shell> shell,
    std::shared_ptr<ms::Surface> const& scene_surface,
    mf::WlRegion* region,
    ConstraintLifetime lifetime) :
    mw::LockedPointerV1{std::move(client), std::move(instance), object_id},
    shell{std::move(shell)},
    weak_scene_surface{scene_surface},
    my_surface_observer{std::make_shared<MyWaylandSurfaceObserver>(this)}
{
    scene_surface->register_interest(my_surface_observer, wayland_executor);

    mir::shell::SurfaceSpecification mods;

    switch (lifetime)
    {
    case ConstraintLifetime::oneshot:
        mods.confine_pointer = MirPointerConfinementState::mir_pointer_locked_oneshot;
        break;

    case ConstraintLifetime::persistent:
        mods.confine_pointer = MirPointerConfinementState::mir_pointer_locked_persistent;
        break;
    }

    if (region)
    {
        auto shape = region->rectangle_vector();
        mods.input_shape = {shape};
    }

    // TODO we need to be able to report "already constrained"
    this->shell->modify_surface(scene_surface->session().lock(), scene_surface, mods);

    if (scene_surface->focus_state() == mir_window_focus_state_focused)
        send_locked_event();
}

LockedPointerV1::~LockedPointerV1()
{
    mark_destroyed();
    if (auto const scene_surface = weak_scene_surface.lock())
    {
        scene_surface->unregister_interest(*my_surface_observer);
        mir::shell::SurfaceSpecification mods;
        mods.confine_pointer = MirPointerConfinementState::mir_pointer_unconfined;
        shell->modify_surface(scene_surface->session().lock(), scene_surface, mods);
    }
}

void LockedPointerV1::set_cursor_position_hint(double /*surface_x*/, double /*surface_y*/)
{
}

void LockedPointerV1::set_region(std::optional<mw::Weak<mw::Region>> const& /*region*/)
{
}

ConfinedPointerV1::ConfinedPointerV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::ConfinedPointerV1Middleware> instance,
    uint32_t object_id,
    mir::Executor& wayland_executor,
    std::shared_ptr<mir::shell::Shell> shell,
    std::shared_ptr<ms::Surface> const& scene_surface,
    mf::WlRegion* region,
    ConstraintLifetime lifetime) :
    mw::ConfinedPointerV1{std::move(client), std::move(instance), object_id},
    shell{std::move(shell)},
    weak_scene_surface(scene_surface),
    my_surface_observer{std::make_shared<SurfaceObserver>(this)}
{
    scene_surface->register_interest(my_surface_observer, wayland_executor);

    mir::shell::SurfaceSpecification mods;

    switch (lifetime)
    {
    case ConstraintLifetime::oneshot:
        mods.confine_pointer = MirPointerConfinementState::mir_pointer_confined_oneshot;
        break;

    case ConstraintLifetime::persistent:
        mods.confine_pointer = MirPointerConfinementState::mir_pointer_confined_persistent;
        break;
    }

    if (region)
    {
        auto shape = region->rectangle_vector();
        mods.input_shape = {shape};
    }

    this->shell->modify_surface(scene_surface->session().lock(), scene_surface, mods);

    if (scene_surface->focus_state() == mir_window_focus_state_focused)
        send_confined_event();
}

ConfinedPointerV1::~ConfinedPointerV1()
{
    mark_destroyed();
    if (auto const scene_surface = weak_scene_surface.lock())
    {
        scene_surface->unregister_interest(*my_surface_observer);
        mir::shell::SurfaceSpecification mods;
        mods.confine_pointer = MirPointerConfinementState::mir_pointer_unconfined;
        shell->modify_surface(scene_surface->session().lock(), scene_surface, mods);
    }
}

void ConfinedPointerV1::set_region(std::optional<mw::Weak<mw::Region>> const& /*region*/)
{
}
}

mf::PointerConstraintsV1::PointerConstraintsV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::PointerConstraintsV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<shell::Shell> shell) :
    mw::PointerConstraintsV1{std::move(client), std::move(instance), object_id},
    wayland_executor{std::move(wayland_executor)},
    shell{std::move(shell)}
{
}

auto mf::PointerConstraintsV1::lock_pointer(
    mw::Weak<mw::Surface> const& surface,
    mw::Weak<mw::Pointer> const& /*pointer*/,
    std::optional<mw::Weak<mw::Region>> const& region,
    uint32_t lifetime,
    rust::Box<mw::LockedPointerV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mw::LockedPointerV1>
{
    auto* const wl_surface = mw::Surface::from<WlSurface>(surface);
    if (!wl_surface)
        return nullptr;

    auto const s = wl_surface->scene_surface();
    if (!s || !s.value())
        return nullptr;

    auto* const wl_region = region ? mw::Region::from<WlRegion>(region.value()) : nullptr;

    // TODO we need to be able to report "already constrained"
    return std::make_shared<LockedPointerV1>(
        client,
        std::move(child_instance),
        child_object_id,
        *wayland_executor,
        shell,
        s.value(),
        wl_region,
        ConstraintLifetime{lifetime});
}

auto mf::PointerConstraintsV1::confine_pointer(
    mw::Weak<mw::Surface> const& surface,
    mw::Weak<mw::Pointer> const& /*pointer*/,
    std::optional<mw::Weak<mw::Region>> const& region,
    uint32_t lifetime,
    rust::Box<mw::ConfinedPointerV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mw::ConfinedPointerV1>
{
    auto* const wl_surface = mw::Surface::from<WlSurface>(surface);
    if (!wl_surface)
        return nullptr;

    auto const s = wl_surface->scene_surface();
    if (!s || !s.value())
        return nullptr;

    auto* const wl_region = region ? mw::Region::from<WlRegion>(region.value()) : nullptr;

    // TODO we need to be able to report "already constrained"
    return std::make_shared<ConfinedPointerV1>(
        client,
        std::move(child_instance),
        child_object_id,
        *wayland_executor,
        shell,
        s.value(),
        wl_region,
        ConstraintLifetime{lifetime});
}
