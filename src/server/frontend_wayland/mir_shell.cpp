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

#include "mir_shell.h"

#include "mir/wayland/protocol_error.h"
#include "wl_surface.h"

namespace mf = mir::frontend;
namespace mw = mir::wayland;

using mir::shell::SurfaceSpecification;
using namespace mir::geometry;

namespace
{
mw::Resource::Version<1> version;

struct Global : mw::MirShellV1::Global
{
    Global(struct wl_display* display) :
        mw::MirShellV1::Global{display, Version<1>{}} {}

    void bind(wl_resource* new_zmir_mir_shell_v1) override;
};

struct Instance : mw::MirShellV1
{
    Instance(struct wl_resource* resource) :
        MirShellV1{resource, version} {}

    void get_normal_surface(struct wl_resource* id, struct wl_resource* surface) override;

    void get_utility_surface(struct wl_resource* id, struct wl_resource* surface) override;

    void get_dialog_surface(struct wl_resource* id, struct wl_resource* surface) override;

    void
    get_satellite_surface(struct wl_resource* id, struct wl_resource* surface, struct wl_resource* positioner) override;

    void get_freestyle_surface(
        struct wl_resource* id, struct wl_resource* surface,
        std::optional<struct wl_resource*> const& positioner) override;

    void create_positioner(struct wl_resource* id) override;
};

template<typename MirSurface, MirWindowType type>
struct Surface : MirSurface
{
    Surface(wl_resource* resource, mf::WlSurface* wl_surface) :
        MirSurface{resource, version}
    {
        SurfaceSpecification spec;
        spec.type = type;
        wl_surface->update_surface_spec(spec);
    }
};

template<typename MirSurface, MirWindowType type>
struct SurfaceWithPosition : MirSurface
{
    SurfaceWithPosition(wl_resource* resource, mf::WlSurface* wl_surface, wl_resource* positioner) :
        MirSurface{resource, version},
        wl_surface{wl_surface}
    {
        position(positioner);
    }

    void reposition(struct wl_resource* positioner, uint32_t /*token*/) override
    {
        position(positioner);
    }

    void position(wl_resource* positioner) const
    {
        auto pspec = dynamic_cast<SurfaceSpecification*>(mw::MirPositionerV1::from(positioner));
        auto spec = pspec ? *pspec : SurfaceSpecification{};

        spec.type = type;
        wl_surface->update_surface_spec(spec);
    }

    mf::WlSurface* const wl_surface;
};

class MirPositioner : public mw::MirPositionerV1, public SurfaceSpecification
{
public:
    MirPositioner(wl_resource* new_resource);

private:
    void set_size(int32_t width, int32_t height) override;
    void set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void set_anchor(uint32_t anchor) override;
    void set_gravity(uint32_t gravity) override;
    void set_constraint_adjustment(uint32_t constraint_adjustment) override;
    void set_offset(int32_t x, int32_t y) override;
};
}

auto mf::create_mir_shell_v1(struct wl_display* display) -> std::shared_ptr<mw::MirShellV1::Global>
{
    return std::make_shared<Global>(display);
}

void Global::bind(wl_resource* new_zmir_mir_shell_v1)
{
    new Instance{new_zmir_mir_shell_v1};
}

void Instance::get_normal_surface(struct wl_resource* id, struct wl_resource* surface)
{
    new Surface<mw::MirNormalSurfaceV1, mir_window_type_normal>{id, mf::WlSurface::from(surface)};
}

void Instance::get_utility_surface(struct wl_resource* id, struct wl_resource* surface)
{
    new Surface<mw::MirUtilitySurfaceV1, mir_window_type_utility>{id, mf::WlSurface::from(surface)};
}

void Instance::get_dialog_surface(struct wl_resource* id, struct wl_resource* surface)
{
    new Surface<mw::MirDialogSurfaceV1, mir_window_type_dialog>{id, mf::WlSurface::from(surface)};
}

void Instance::get_satellite_surface(wl_resource* id, wl_resource* surface, wl_resource* positioner)
{
    new SurfaceWithPosition<mw::MirSatelliteSurfaceV1, mir_window_type_satellite>
        {id, mf::WlSurface::from(surface), positioner};
}

void Instance::get_freestyle_surface(
    wl_resource* id, wl_resource* surface, std::optional<struct wl_resource*> const& positioner)
{
    new SurfaceWithPosition<mw::MirFreestyleSurfaceV1, mir_window_type_freestyle>
        {id, mf::WlSurface::from(surface), positioner.value_or(nullptr)};
}

void Instance::create_positioner(struct wl_resource* id)
{
    new MirPositioner(id);
}

MirPositioner::MirPositioner(wl_resource* new_resource)
    : mw::MirPositionerV1(new_resource, Version<1>())
{
    // specifying gravity is not required by the xdg shell protocol, but is by Mir window managers
    surface_placement_gravity = mir_placement_gravity_center;
    aux_rect_placement_gravity = mir_placement_gravity_center;
    placement_hints = static_cast<MirPlacementHints>(0);
}

void MirPositioner::set_size(int32_t width, int32_t height)
{
    if (width <= 0 || height <= 0)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            mw::MirPositionerV1::Error::invalid_input,
            "Invalid popup positioner size: %dx%d", width, height));
    }
    this->width = Width{width};
    this->height = Height{height};
}

void MirPositioner::set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (width < 0 || height < 0)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            mw::MirPositionerV1::Error::invalid_input,
            "Invalid popup anchor rect size: %dx%d", width, height));
    }
    aux_rect = Rectangle{{x, y}, {width, height}};
}

void MirPositioner::set_anchor(uint32_t anchor)
{
    MirPlacementGravity placement = mir_placement_gravity_center;

    switch (anchor)
    {
    case Anchor::top:
        placement = mir_placement_gravity_north;
        break;

    case Anchor::bottom:
        placement = mir_placement_gravity_south;
        break;

    case Anchor::left:
        placement = mir_placement_gravity_west;
        break;

    case Anchor::right:
        placement = mir_placement_gravity_east;
        break;

    case Anchor::top_left:
        placement = mir_placement_gravity_northwest;
        break;

    case Anchor::bottom_left:
        placement = mir_placement_gravity_southwest;
        break;

    case Anchor::top_right:
        placement = mir_placement_gravity_northeast;
        break;

    case Anchor::bottom_right:
        placement = mir_placement_gravity_southeast;
        break;

    default:
        placement = mir_placement_gravity_center;
    }

    aux_rect_placement_gravity = placement;
}

void MirPositioner::set_gravity(uint32_t gravity)
{
    MirPlacementGravity placement = mir_placement_gravity_center;

    switch (gravity)
    {
    case Gravity::top:
        placement = mir_placement_gravity_south;
        break;

    case Gravity::bottom:
        placement = mir_placement_gravity_north;
        break;

    case Gravity::left:
        placement = mir_placement_gravity_east;
        break;

    case Gravity::right:
        placement = mir_placement_gravity_west;
        break;

    case Gravity::top_left:
        placement = mir_placement_gravity_southeast;
        break;

    case Gravity::bottom_left:
        placement = mir_placement_gravity_northeast;
        break;

    case Gravity::top_right:
        placement = mir_placement_gravity_southwest;
        break;

    case Gravity::bottom_right:
        placement = mir_placement_gravity_northwest;
        break;

    case Gravity::none:
        placement = mir_placement_gravity_center;
        break;

    default:
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            mw::MirPositionerV1::Error::invalid_input,
            "Invalid gravity value %d", gravity));
    }

    surface_placement_gravity = placement;
}

void MirPositioner::set_constraint_adjustment(uint32_t constraint_adjustment)
{
    uint32_t new_placement_hints{0};
    if (constraint_adjustment & ConstraintAdjustment::slide_x)
    {
        new_placement_hints |= mir_placement_hints_slide_x;
    }
    if (constraint_adjustment & ConstraintAdjustment::slide_y)
    {
        new_placement_hints |= mir_placement_hints_slide_y;
    }
    if (constraint_adjustment & ConstraintAdjustment::flip_x)
    {
        new_placement_hints |= mir_placement_hints_flip_x;
    }
    if (constraint_adjustment & ConstraintAdjustment::flip_x)
    {
        new_placement_hints |= mir_placement_hints_flip_y;
    }
    if (constraint_adjustment & ConstraintAdjustment::resize_x)
    {
        new_placement_hints |= mir_placement_hints_resize_x;
    }
    if (constraint_adjustment & ConstraintAdjustment::resize_y)
    {
        new_placement_hints |= mir_placement_hints_resize_y;
    }
    placement_hints = static_cast<MirPlacementHints>(new_placement_hints);
}

void MirPositioner::set_offset(int32_t x, int32_t y)
{
    aux_rect_placement_offset_x = x;
    aux_rect_placement_offset_y = y;
}
