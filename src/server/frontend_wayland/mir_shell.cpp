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
#include "weak.h"
#include "wl_surface.h"

namespace mf = mir::frontend;
namespace mw = mir::wayland;

using mir::shell::SurfaceSpecification;
using namespace mir::geometry;

namespace
{
class MirPositioner : public mw::MirPositionerV1, public SurfaceSpecification
{
public:
    MirPositioner(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::MirPositionerV1Middleware> instance,
        uint32_t object_id);

private:
    void set_size(int32_t width, int32_t height) override;
    void set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void set_anchor(uint32_t anchor) override;
    void set_gravity(uint32_t gravity) override;
    void set_constraint_adjustment(uint32_t constraint_adjustment) override;
    void set_offset(int32_t x, int32_t y) override;
};

template<typename MirSurface, typename Middleware, MirWindowType type>
struct Surface : MirSurface
{
    Surface(
        std::shared_ptr<mw::Client> client,
        rust::Box<Middleware> instance,
        uint32_t object_id,
        mf::WlSurface* wl_surface) :
        MirSurface{std::move(client), std::move(instance), object_id}
    {
        SurfaceSpecification spec;
        spec.type = type;
        wl_surface->update_surface_spec(spec);
    }
};

template<typename MirSurface, typename Middleware, MirWindowType type>
struct SurfaceWithPosition : MirSurface
{
    SurfaceWithPosition(
        std::shared_ptr<mw::Client> client,
        rust::Box<Middleware> instance,
        uint32_t object_id,
        mf::WlSurface* wl_surface,
        mw::Weak<mw::MirPositionerV1> const& positioner) :
        MirSurface{std::move(client), std::move(instance), object_id},
        wl_surface{wl_surface}
    {
        position(positioner);
    }

    using MirSurface::reposition;
    auto reposition(mw::Weak<mw::MirPositionerV1> const& positioner, uint32_t /*token*/) -> void override
    {
        position(positioner);
    }

    void position(mw::Weak<mw::MirPositionerV1> const& positioner) const
    {
        auto pspec = mw::MirPositionerV1::from<MirPositioner>(positioner);
        auto spec = pspec ? *pspec : SurfaceSpecification{};

        spec.type = type;
        wl_surface->update_surface_spec(spec);
    }

    mf::WlSurface* const wl_surface;
};

class MirShellV1 : public mw::MirShellV1
{
public:
    MirShellV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::MirShellV1Middleware> instance,
        uint32_t object_id) :
        mw::MirShellV1{std::move(client), std::move(instance), object_id}
    {
    }

private:
    using mw::MirShellV1::get_regular_surface;
    auto get_regular_surface(
        mw::Weak<mw::Surface> const& surface,
        rust::Box<mw::MirRegularSurfaceV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::MirRegularSurfaceV1> override
    {
        return std::make_shared<Surface<mw::MirRegularSurfaceV1, mw::MirRegularSurfaceV1Middleware, mir_window_type_normal>>(
            client, std::move(child_instance), child_object_id, mw::Surface::from<mf::WlSurface>(surface));
    }

    using mw::MirShellV1::get_floating_regular_surface;
    auto get_floating_regular_surface(
        mw::Weak<mw::Surface> const& surface,
        rust::Box<mw::MirFloatingRegularSurfaceV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::MirFloatingRegularSurfaceV1> override
    {
        struct FloatingSurface : mw::MirFloatingRegularSurfaceV1
        {
            FloatingSurface(
                std::shared_ptr<mw::Client> client,
                rust::Box<mw::MirFloatingRegularSurfaceV1Middleware> instance,
                uint32_t object_id,
                mf::WlSurface* wl_surface) :
                mw::MirFloatingRegularSurfaceV1{std::move(client), std::move(instance), object_id}
            {
                SurfaceSpecification spec;
                spec.type = mir_window_type_utility;
                spec.depth_layer = mir_depth_layer_always_on_top;
                wl_surface->update_surface_spec(spec);
            }
        };

        return std::make_shared<FloatingSurface>(
            client, std::move(child_instance), child_object_id, mw::Surface::from<mf::WlSurface>(surface));
    }

    using mw::MirShellV1::get_dialog_surface;
    auto get_dialog_surface(
        mw::Weak<mw::Surface> const& surface,
        rust::Box<mw::MirDialogSurfaceV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::MirDialogSurfaceV1> override
    {
        return std::make_shared<Surface<mw::MirDialogSurfaceV1, mw::MirDialogSurfaceV1Middleware, mir_window_type_dialog>>(
            client, std::move(child_instance), child_object_id, mw::Surface::from<mf::WlSurface>(surface));
    }

    using mw::MirShellV1::get_satellite_surface;
    auto get_satellite_surface(
        mw::Weak<mw::Surface> const& surface,
        mw::Weak<mw::MirPositionerV1> const& positioner,
        rust::Box<mw::MirSatelliteSurfaceV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::MirSatelliteSurfaceV1> override
    {
        return std::make_shared<
            SurfaceWithPosition<mw::MirSatelliteSurfaceV1, mw::MirSatelliteSurfaceV1Middleware, mir_window_type_satellite>>(
            client, std::move(child_instance), child_object_id, mw::Surface::from<mf::WlSurface>(surface), positioner);
    }

    auto create_positioner(
        rust::Box<mw::MirPositionerV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::MirPositionerV1> override
    {
        return std::make_shared<MirPositioner>(client, std::move(child_instance), child_object_id);
    }
};
}

auto mf::create_mir_shell_v1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::MirShellV1Middleware> instance,
    uint32_t object_id) -> std::shared_ptr<mw::MirShellV1>
{
    return std::make_shared<MirShellV1>(std::move(client), std::move(instance), object_id);
}

MirPositioner::MirPositioner(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::MirPositionerV1Middleware> instance,
    uint32_t object_id)
    : mw::MirPositionerV1{std::move(client), std::move(instance), object_id}
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
        throw mw::ProtocolError{
            object_id(),
            mw::MirPositionerV1::Error::invalid_input,
            "Invalid popup positioner size: %dx%d", width, height};
    }
    this->width = Width{width};
    this->height = Height{height};
}

void MirPositioner::set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (width < 0 || height < 0)
    {
        throw mw::ProtocolError{
            object_id(),
            mw::MirPositionerV1::Error::invalid_input,
            "Invalid popup anchor rect size: %dx%d", width, height};
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
        throw mw::ProtocolError{
            object_id(),
            mw::MirPositionerV1::Error::invalid_input,
            "Invalid gravity value %d", gravity};
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
