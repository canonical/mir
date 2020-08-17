/*
 * Copyright © 2020 Canonical Ltd.
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
#include "pointer-constraints-unstable-v1_wrapper.h"
#include "wl_region.h"
#include "wl_surface.h"

#include <mir/scene/surface.h>
#include <mir/shell/shell.h>
#include <mir/shell/surface_specification.h>

namespace mir
{
namespace frontend
{
class PointerConstraintsV1 : public wayland::PointerConstraintsV1
{
public:
    PointerConstraintsV1(wl_resource* resource, std::shared_ptr<shell::Shell> shell);

    class Global : public wayland::PointerConstraintsV1::Global
    {
    public:
        Global(wl_display* display, std::shared_ptr<shell::Shell> shell);

    private:
        void bind(wl_resource* new_zwp_pointer_constraints_v1) override;
        std::shared_ptr<shell::Shell> const shell;
    };

private:
    std::shared_ptr<shell::Shell> const shell;

    void destroy() override;

    void lock_pointer(wl_resource* id,
                      wl_resource* surface,
                      wl_resource* pointer,
                      const std::experimental::optional<wl_resource*>& region,
                      uint32_t lifetime) override;

    void confine_pointer(wl_resource* id,
                         wl_resource* surface,
                         wl_resource* pointer,
                         const std::experimental::optional<wl_resource*>& region,
                         uint32_t lifetime) override;
};

class LockedPointerV1 : public wayland::LockedPointerV1
{
public:
    LockedPointerV1(
        wl_resource* id,
        std::shared_ptr<shell::Shell> shell,
        std::shared_ptr<scene::Surface> const& scene_surface,
        std::experimental::optional<wl_resource*> const& region);

private:
    std::shared_ptr<shell::Shell> const shell;
    std::weak_ptr<scene::Surface> const weak_scene_surface;

    void destroy() override;
    void set_cursor_position_hint(double /*surface_x*/, double /*surface_y*/) override;
    void set_region(const std::experimental::optional<wl_resource*>& /*region*/) override;
};

class ConfinedPointerV1 : public wayland::ConfinedPointerV1
{
public:
    ConfinedPointerV1(
        wl_resource* id,
        std::shared_ptr<shell::Shell> shell,
        std::shared_ptr<scene::Surface> const& scene_surface,
        std::experimental::optional<wl_resource*> const& region);

private:
    std::shared_ptr<shell::Shell> const shell;
    std::weak_ptr<scene::Surface> const weak_scene_surface;
    void destroy() override;
    void set_region(const std::experimental::optional<wl_resource*>& /*region*/) override;
};
}
}

auto mir::frontend::create_pointer_constraints_unstable_v1(wl_display* display, std::shared_ptr<shell::Shell> shell)
    -> std::shared_ptr<void>
{
    puts(__PRETTY_FUNCTION__);
    return std::make_shared<PointerConstraintsV1::Global>(display, std::move(shell));
}

mir::frontend::PointerConstraintsV1::Global::Global(wl_display* display, std::shared_ptr<shell::Shell> shell) :
    wayland::PointerConstraintsV1::Global::Global{display, Version<1>{}},
    shell{std::move(shell)}
{
    puts(__PRETTY_FUNCTION__);
}

void mir::frontend::PointerConstraintsV1::Global::bind(wl_resource* new_zwp_pointer_constraints_v1)
{
    puts(__PRETTY_FUNCTION__);
    new PointerConstraintsV1{new_zwp_pointer_constraints_v1, shell};
}

mir::frontend::PointerConstraintsV1::PointerConstraintsV1(wl_resource* resource, std::shared_ptr<shell::Shell> shell) :
    wayland::PointerConstraintsV1{resource, Version<1>{}},
    shell{std::move(shell)}
{
    puts(__PRETTY_FUNCTION__);
}

void mir::frontend::PointerConstraintsV1::destroy()
{
    puts(__PRETTY_FUNCTION__);
    destroy_wayland_object();
}

void mir::frontend::PointerConstraintsV1::lock_pointer(wl_resource* id,
                                                       wl_resource* surface,
                                                       wl_resource* /*pointer*/,
                                                       std::experimental::optional<wl_resource*> const& region,
                                                       uint32_t /*lifetime*/)
{
    puts(__PRETTY_FUNCTION__);
    if (auto const s = WlSurface::from(surface)->scene_surface())
    {
        if (auto const ss = s.value())
        {
            new LockedPointerV1{id, shell, ss, region};
        }
    }
}

void mir::frontend::PointerConstraintsV1::confine_pointer(wl_resource* id,
                                                          wl_resource* surface,
                                                          wl_resource* /*pointer*/,
                                                          std::experimental::optional<wl_resource*> const& region,
                                                          uint32_t /*lifetime*/)
{
    puts(__PRETTY_FUNCTION__);
    if (auto const s = WlSurface::from(surface)->scene_surface())
    {
        if (auto const ss = s.value())
        {
            new ConfinedPointerV1{id, shell, ss, region};
        }
    }
}

mir::frontend::LockedPointerV1::LockedPointerV1(
    wl_resource* id,
    std::shared_ptr<shell::Shell> shell,
    std::shared_ptr<scene::Surface> const& scene_surface,
    std::experimental::optional<wl_resource*> const& region) :
      wayland::LockedPointerV1{id, Version<1>{}},
      shell{std::move(shell)},
      weak_scene_surface{scene_surface}
{
    puts(__PRETTY_FUNCTION__);

    shell::SurfaceSpecification mods;
    mods.confine_pointer = MirPointerConfinementState::mir_pointer_confined_to_window;

    if (region)
    {
        if (WlRegion* wlregion = WlRegion::from(region.value()))
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
    send_locked_event();
}

void mir::frontend::LockedPointerV1::destroy()
{
    puts(__PRETTY_FUNCTION__);
    send_unlocked_event();
    destroy_wayland_object();

    if (auto const scene_surface = weak_scene_surface.lock())
    {
        shell::SurfaceSpecification mods;
        mods.confine_pointer = MirPointerConfinementState::mir_pointer_unconfined;
        shell->modify_surface(scene_surface->session().lock(), scene_surface, mods);
    }
}

void mir::frontend::LockedPointerV1::set_cursor_position_hint(double /*surface_x*/, double /*surface_y*/)
{
    puts(__PRETTY_FUNCTION__);
}

void mir::frontend::LockedPointerV1::set_region(const std::experimental::optional<wl_resource*>& /*region*/)
{
    puts(__PRETTY_FUNCTION__);
}

mir::frontend::ConfinedPointerV1::ConfinedPointerV1(
    wl_resource* id,
    std::shared_ptr<shell::Shell> shell,
    std::shared_ptr<scene::Surface> const& scene_surface,
    std::experimental::optional<wl_resource*> const& region) :
    wayland::ConfinedPointerV1{id, Version<1>{}},
    shell{std::move(shell)},
    weak_scene_surface(scene_surface)
{
    puts(__PRETTY_FUNCTION__);

    shell::SurfaceSpecification mods;
    mods.confine_pointer = MirPointerConfinementState::mir_pointer_confined_to_window;

    if (region)
    {
        if (WlRegion* wlregion = WlRegion::from(region.value()))
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
}

void mir::frontend::ConfinedPointerV1::destroy()
{
    puts(__PRETTY_FUNCTION__);
    if (auto const scene_surface = weak_scene_surface.lock())
    {
        shell::SurfaceSpecification mods;
        mods.confine_pointer = MirPointerConfinementState::mir_pointer_unconfined;
        shell->modify_surface(scene_surface->session().lock(), scene_surface, mods);
    }

    destroy_wayland_object();
}

void mir::frontend::ConfinedPointerV1::set_region(const std::experimental::optional<wl_resource*>& /*region*/)
{
    puts(__PRETTY_FUNCTION__);
}
