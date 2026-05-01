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
#include "protocol_error.h"
#include "wl_surface.h"

namespace mf = mir::frontend;
namespace mw = mir::wayland_rs;

using mir::shell::SurfaceSpecification;
using namespace mir::geometry;

namespace
{

template<typename MirSurface, MirWindowType type>
struct Surface : MirSurface
{
    Surface(mf::WlSurface* wl_surface)
    {
        SurfaceSpecification spec;
        spec.type = type;
        wl_surface->update_surface_spec(spec);
    }
};

template<typename MirSurface, MirWindowType type>
struct SurfaceWithPosition : MirSurface
{
    SurfaceWithPosition(mf::WlSurface* wl_surface, mw::MirPositionerV1Impl* positioner) :
        wl_surface{wl_surface}
    {
        position(positioner);
    }

    void reposition(mw::Weak<mw::MirPositionerV1Impl> const& positioner, uint32_t /*token*/) override
    {
        position(&positioner.value());
    }

    void position(mw::MirPositionerV1Impl* positioner) const
    {
        auto pspec = dynamic_cast<SurfaceSpecification*>(positioner);
        auto spec = pspec ? *pspec : SurfaceSpecification{};

        spec.type = type;
        wl_surface->update_surface_spec(spec);
    }

    mf::WlSurface* const wl_surface;
};

class MirPositioner : public mw::MirPositionerV1Impl, public SurfaceSpecification
{
public:
    MirPositioner();
    void set_size(int32_t width, int32_t height) override;
    void set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void set_anchor(uint32_t anchor) override;
    void set_gravity(uint32_t gravity) override;
    void set_constraint_adjustment(uint32_t constraint_adjustment) override;
    void set_offset(int32_t x, int32_t y) override;
};
}


auto mf::MirShellV1::get_regular_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface)
    -> std::shared_ptr<wayland_rs::MirRegularSurfaceV1Impl>
{
    using RegularSurface = ::Surface<mw::MirRegularSurfaceV1Impl, mir_window_type_normal>;
    return std::make_shared<RegularSurface>(mf::WlSurface::from(&surface.value()));
}

auto mf::MirShellV1::get_floating_regular_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface)
    -> std::shared_ptr<wayland_rs::MirFloatingRegularSurfaceV1Impl>
{
    struct Surface : mw::MirFloatingRegularSurfaceV1Impl
    {
        Surface(WlSurface* wl_surface)
        {
            SurfaceSpecification spec;
            spec.type = mir_window_type_utility;
            spec.depth_layer = mir_depth_layer_always_on_top;
            wl_surface->update_surface_spec(spec);
        }
    };

    return std::make_shared<Surface>(WlSurface::from(&surface.value()));
}

auto mf::MirShellV1::get_dialog_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface)
    -> std::shared_ptr<wayland_rs::MirDialogSurfaceV1Impl>
{
    return std::make_shared<::Surface<mw::MirDialogSurfaceV1Impl, mir_window_type_dialog>>(mf::WlSurface::from(&surface.value()));
}

auto mf::MirShellV1::get_satellite_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface,
    wayland_rs::Weak<wayland_rs::MirPositionerV1Impl> const& positioner)
    -> std::shared_ptr<wayland_rs::MirSatelliteSurfaceV1Impl>
{
    using SatelliteType = ::SurfaceWithPosition<mw::MirSatelliteSurfaceV1Impl, mir_window_type_satellite>;
    return std::make_shared<SatelliteType>(
        WlSurface::from(&surface.value()), &positioner.value());
}

auto mf::MirShellV1::create_positioner() -> std::shared_ptr<wayland_rs::MirPositionerV1Impl>
{
    return std::make_shared<MirPositioner>();
}

MirPositioner::MirPositioner()
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
            object_id(),
            mw::MirPositionerV1Impl::Error::invalid_input,
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
            object_id(),
            mw::MirPositionerV1Impl::Error::invalid_input,
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
            object_id(),
            mw::MirPositionerV1Impl::Error::invalid_input,
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
    if (constraint_adjustment & ConstraintAdjustment::flip_y)
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
