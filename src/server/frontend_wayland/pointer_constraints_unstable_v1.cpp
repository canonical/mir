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

#include "pointer_constraints_unstable_v1.h"
#include "wl_region.h"
#include "wl_surface.h"

#include <mir/executor.h>
#include <mir/scene/surface.h>
#include <mir/scene/null_surface_observer.h>
#include <mir/shell/shell.h>
#include <mir/shell/surface_specification.h>

namespace mw = mir::wayland_rs;
namespace ms = mir::scene;
namespace mf = mir::frontend;

namespace mir
{
namespace frontend
{
class LockedPointerV1 : public mw::ZwpLockedPointerV1Impl, public std::enable_shared_from_this<LockedPointerV1>
{
public:
    LockedPointerV1(
        Executor& wayland_executor,
        std::shared_ptr<shell::Shell> shell,
        std::shared_ptr<scene::Surface> const& scene_surface,
        mw::Weak<mw::WlRegionImpl> const& region,
        PointerConstraintsV1::Lifetime lifetime);
    ~LockedPointerV1() override;

    auto associate(rust::Box<mw::ZwpLockedPointerV1Ext> instance, uint32_t object_id) -> void override;
    auto set_region(wayland_rs::Weak<wayland_rs::WlRegionImpl> const& region, bool has_region) -> void override;
    void set_cursor_position_hint(double /*surface_x*/, double /*surface_y*/) override;

private:
    Executor& wayland_executor;
    std::shared_ptr<shell::Shell> const shell;
    std::weak_ptr<scene::Surface> const weak_scene_surface;
    mw::Weak<mw::WlRegionImpl> const initial_region;
    PointerConstraintsV1::Lifetime const lifetime;

    struct MyWaylandSurfaceObserver;

    std::shared_ptr<MyWaylandSurfaceObserver> my_surface_observer;
};

class ConfinedPointerV1 : public mw::ZwpConfinedPointerV1Impl, std::enable_shared_from_this<ConfinedPointerV1>
{
public:
    ConfinedPointerV1(
        Executor& wayland_executor,
        std::shared_ptr<shell::Shell> shell,
        std::shared_ptr<scene::Surface> const& scene_surface,
        mw::Weak<mw::WlRegionImpl> const& region,
        PointerConstraintsV1::Lifetime lifetime);
    ~ConfinedPointerV1() override;

    auto associate(rust::Box<mw::ZwpConfinedPointerV1Ext> instance, uint32_t object_id) -> void override;
    auto set_region(wayland_rs::Weak<wayland_rs::WlRegionImpl> const& region, bool has_region) -> void override;

private:
    Executor& wayland_executor;
    std::shared_ptr<shell::Shell> const shell;
    std::weak_ptr<scene::Surface> const weak_scene_surface;
    mw::Weak<mw::WlRegionImpl> const initial_region;
    PointerConstraintsV1::Lifetime const lifetime;

    struct SurfaceObserver;

    std::shared_ptr<SurfaceObserver> my_surface_observer;
};

struct LockedPointerV1::MyWaylandSurfaceObserver : ms::NullSurfaceObserver
{
    MyWaylandSurfaceObserver(std::shared_ptr<LockedPointerV1> const& self) : self{self} {}

    void attrib_changed(const scene::Surface* surf, MirWindowAttrib attrib, int value) override;
    mw::Weak<LockedPointerV1> const self;
};

void LockedPointerV1::MyWaylandSurfaceObserver::attrib_changed(
    scene::Surface const* surf,
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
    SurfaceObserver(std::shared_ptr<ConfinedPointerV1> const& self) : self{self} {}

    void attrib_changed(const scene::Surface* surf, MirWindowAttrib attrib, int value) override;
    mw::Weak<ConfinedPointerV1> const self;
};

void ConfinedPointerV1::SurfaceObserver::attrib_changed(
    scene::Surface const* surf,
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
}
}

mir::frontend::PointerConstraintsV1::PointerConstraintsV1(Executor& wayland_executor, std::shared_ptr<shell::Shell> shell) :
    wayland_executor{wayland_executor},
    shell{std::move(shell)}
{
}

auto mir::frontend::PointerConstraintsV1::lock_pointer(
    mw::Weak<mw::WlSurfaceImpl> const& surface,
    mw::Weak<mw::WlPointerImpl> const&,
    mw::Weak<mw::WlRegionImpl> const& region,
    bool /*has_region*/,
    uint32_t lifetime) -> std::shared_ptr<mw::ZwpLockedPointerV1Impl>
{
    if (auto const s = WlSurface::from(&surface.value())->scene_surface())
    {
        if (auto const ss = s.value())
        {
            // TODO we need to be able to report "already constrained"
            return std::make_shared<LockedPointerV1>(wayland_executor, shell, ss, region, Lifetime{lifetime});
        }
    }

    return nullptr;
}

auto mir::frontend::PointerConstraintsV1::confine_pointer(
    mw::Weak<mw::WlSurfaceImpl> const& surface,
    mw::Weak<mw::WlPointerImpl> const&,
    mw::Weak<mw::WlRegionImpl> const& region,
    bool /*has_region*/,
    uint32_t lifetime) -> std::shared_ptr<mw::ZwpConfinedPointerV1Impl>
{
    if (auto const s = WlSurface::from(&surface.value())->scene_surface())
    {
        if (auto const ss = s.value())
        {
            // TODO we need to be able to report "already constrained"
            return std::make_shared<ConfinedPointerV1>(wayland_executor, shell, ss, region, Lifetime{lifetime});
        }
    }

    return nullptr;
}

mir::frontend::LockedPointerV1::LockedPointerV1(
    Executor& wayland_executor,
    std::shared_ptr<shell::Shell> shell,
    std::shared_ptr<scene::Surface> const& scene_surface,
    mw::Weak<mw::WlRegionImpl> const& region,
    PointerConstraintsV1::Lifetime lifetime) :
    wayland_executor{wayland_executor},
    shell{std::move(shell)},
    weak_scene_surface{scene_surface},
    initial_region{region},
    lifetime{lifetime}
{
}

auto mir::frontend::LockedPointerV1::associate(rust::Box<mw::ZwpLockedPointerV1Ext> instance, uint32_t object_id) -> void
{
    ZwpLockedPointerV1Impl::associate(std::move(instance), object_id);

    my_surface_observer = std::make_shared<MyWaylandSurfaceObserver>(shared_from_this());

    auto const scene_surface = weak_scene_surface.lock();
    if (!scene_surface)
        return;

    scene_surface->register_interest(my_surface_observer, wayland_executor);

    shell::SurfaceSpecification mods;

    switch (lifetime)
    {
    case PointerConstraintsV1::Lifetime::oneshot:
        mods.confine_pointer = MirPointerConfinementState::mir_pointer_locked_oneshot;
        break;

    case PointerConstraintsV1::Lifetime::persistent:
        mods.confine_pointer = MirPointerConfinementState::mir_pointer_locked_persistent;
        break;
    }

    if (initial_region)
    {
        if (WlRegion* wlregion = WlRegion::from(&initial_region.value()))
        {
            auto shape = wlregion->rectangle_vector();
            mods.input_shape = {shape};
        }
        else
        {
            mods.input_shape = std::vector<geometry::Rectangle>{};
        }
    }

    // TODO we need to be able to report "already constrained"
    this->shell->modify_surface(scene_surface->session().lock(), scene_surface, mods);

    if (scene_surface->focus_state() == mir_window_focus_state_focused)
        send_locked_event();
}

mir::frontend::LockedPointerV1::~LockedPointerV1()
{
    mark_destroyed();
    if (auto const scene_surface = weak_scene_surface.lock())
    {
        if (my_surface_observer)
            scene_surface->unregister_interest(*my_surface_observer);
        shell::SurfaceSpecification mods;
        mods.confine_pointer = MirPointerConfinementState::mir_pointer_unconfined;
        shell->modify_surface(scene_surface->session().lock(), scene_surface, mods);
    }
}

void mf::LockedPointerV1::set_cursor_position_hint(double /*surface_x*/, double /*surface_y*/)
{
}

auto mir::frontend::LockedPointerV1::set_region(wayland_rs::Weak<wayland_rs::WlRegionImpl> const&, bool) -> void
{
}

mir::frontend::ConfinedPointerV1::ConfinedPointerV1(
    Executor& wayland_executor,
    std::shared_ptr<shell::Shell> shell,
    std::shared_ptr<scene::Surface> const& scene_surface,
    wayland_rs::Weak<mw::WlRegionImpl> const& region,
    PointerConstraintsV1::Lifetime lifetime) :
    wayland_executor{wayland_executor},
    shell{std::move(shell)},
    weak_scene_surface(scene_surface),
    initial_region{region},
    lifetime{lifetime}
{
}

auto mir::frontend::ConfinedPointerV1::associate(rust::Box<mw::ZwpConfinedPointerV1Ext> instance, uint32_t object_id) -> void
{
    ZwpConfinedPointerV1Impl::associate(std::move(instance), object_id);

    my_surface_observer = std::make_shared<SurfaceObserver>(shared_from_this());

    auto const scene_surface = weak_scene_surface.lock();
    if (!scene_surface)
        return;

    scene_surface->register_interest(my_surface_observer, wayland_executor);

    shell::SurfaceSpecification mods;

    switch (lifetime)
    {
    case PointerConstraintsV1::Lifetime::oneshot:
        mods.confine_pointer = MirPointerConfinementState::mir_pointer_confined_oneshot;
        break;

    case PointerConstraintsV1::Lifetime::persistent:
        mods.confine_pointer = MirPointerConfinementState::mir_pointer_confined_persistent;
        break;
    }

    if (initial_region)
    {
        if (WlRegion* wlregion = WlRegion::from(&initial_region.value()))
        {
            auto shape = wlregion->rectangle_vector();
            mods.input_shape = {shape};
        }
        else
        {
            mods.input_shape = std::vector<geometry::Rectangle>{};
        }
    }

    this->shell->modify_surface(scene_surface->session().lock(), scene_surface, mods);

    if (scene_surface->focus_state() == mir_window_focus_state_focused)
        send_confined_event();
}

mir::frontend::ConfinedPointerV1::~ConfinedPointerV1()
{
    mark_destroyed();
    if (auto const scene_surface = weak_scene_surface.lock())
    {
        if (my_surface_observer)
            scene_surface->unregister_interest(*my_surface_observer);
        shell::SurfaceSpecification mods;
        mods.confine_pointer = MirPointerConfinementState::mir_pointer_unconfined;
        shell->modify_surface(scene_surface->session().lock(), scene_surface, mods);
    }
}

auto mir::frontend::ConfinedPointerV1::set_region(wayland_rs::Weak<wayland_rs::WlRegionImpl> const&, bool) -> void
{
}
