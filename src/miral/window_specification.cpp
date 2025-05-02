/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#include "miral/window_specification.h"
#include "window_specification_internal.h"

#include <mir/shell/surface_specification.h>

miral::WindowSpecification::Self::Self(mir::shell::SurfaceSpecification const& spec) :
    top_left(spec.top_left),
    size(),
    pixel_format(spec.pixel_format),
    buffer_usage(),
    name(spec.name),
    output_id(),
    type(spec.type),
    state(spec.state),
    preferred_orientation(spec.preferred_orientation),
    aux_rect(spec.aux_rect),
    placement_hints(spec.placement_hints),
    window_placement_gravity(spec.surface_placement_gravity),
    aux_rect_placement_gravity(spec.aux_rect_placement_gravity),
    min_width(spec.min_width),
    min_height(spec.min_height),
    max_width(spec.max_width),
    max_height(spec.max_height),
    width_inc(spec.width_inc),
    height_inc(spec.height_inc),
    min_aspect(),
    max_aspect(),
    streams(spec.streams),
    parent(spec.parent),
    input_shape(spec.input_shape),
    input_mode(spec.input_mode.is_set() ?
        mir::optional_value<InputReceptionMode>(static_cast<InputReceptionMode>(spec.input_mode.value())) :
        mir::optional_value<InputReceptionMode>()),
    shell_chrome(spec.shell_chrome),
    confine_pointer(spec.confine_pointer),
    depth_layer(spec.depth_layer),
    attached_edges(spec.attached_edges),
    exclusive_rect(spec.exclusive_rect),
    ignore_exclusion_zones(spec.ignore_exclusion_zones),
    application_id(spec.application_id),
    server_side_decorated(spec.server_side_decorated),
    focus_mode(spec.focus_mode),
    visible_on_lock_screen(spec.visible_on_lock_screen),
    tiled_edges(spec.tiled_edges)
{
    if (spec.aux_rect_placement_offset_x.is_set() && spec.aux_rect_placement_offset_y.is_set())
        aux_rect_placement_offset = Displacement{spec.aux_rect_placement_offset_x.value(), spec.aux_rect_placement_offset_y.value()};

    if (spec.edge_attachment.is_set() && !placement_hints.is_set())
    {
        switch (spec.edge_attachment.value())
        {
        case mir_edge_attachment_vertical:
            window_placement_gravity = mir_placement_gravity_northwest;
            aux_rect_placement_gravity = mir_placement_gravity_northeast;
            placement_hints = mir_placement_hints_flip_x;
            break;

        case mir_edge_attachment_horizontal:
            window_placement_gravity = mir_placement_gravity_northwest;
            aux_rect_placement_gravity = mir_placement_gravity_southwest;
            placement_hints = mir_placement_hints_flip_y;
            break;

        case mir_edge_attachment_any:
            window_placement_gravity = mir_placement_gravity_northwest;
            aux_rect_placement_gravity = mir_placement_gravity_northeast;
            placement_hints = MirPlacementHints(mir_placement_hints_flip_any | mir_placement_hints_antipodes);
            break;
        }
    }

    if (spec.width.is_set() && spec.height.is_set())
        size = Size(spec.width.value(), spec.height.value());

    if (spec.buffer_usage.is_set())
        buffer_usage = BufferUsage(spec.buffer_usage.value());

    if (spec.output_id.is_set())
        output_id = spec.output_id.value().as_value();

    if (spec.min_aspect.is_set())
        min_aspect = AspectRatio{spec.min_aspect.value().width, spec.min_aspect.value().height};

    if (spec.max_aspect.is_set())
        max_aspect = AspectRatio{spec.max_aspect.value().width, spec.max_aspect.value().height};
}

miral::WindowSpecification::WindowSpecification() :
    self{std::make_unique<Self>()}
{
}

miral::WindowSpecification::WindowSpecification(mir::shell::SurfaceSpecification const& spec) :
    self{std::make_unique<Self>(spec)}
{
}

miral::WindowSpecification::WindowSpecification(WindowSpecification const& that) :
    self{std::make_unique<Self>(*that.self)}
{
}

auto miral::WindowSpecification::operator=(WindowSpecification const& that) -> WindowSpecification&
{
    self = std::make_unique<Self>(*that.self);
    return *this;
}

miral::WindowSpecification::~WindowSpecification() = default;

auto miral::WindowSpecification::top_left() const -> mir::optional_value<Point> const&
{
    return self->top_left;
}

auto miral::WindowSpecification::size() const -> mir::optional_value<Size> const&
{
    return self->size;
}

auto miral::WindowSpecification::name() const -> mir::optional_value<std::string> const&
{
    return self->name;
}

auto miral::WindowSpecification::output_id() const -> mir::optional_value<int> const&
{
    return self->output_id;
}

auto miral::WindowSpecification::type() const -> mir::optional_value<MirWindowType> const&
{
    return self->type;
}

auto miral::WindowSpecification::state() const -> mir::optional_value<MirWindowState> const&
{
    return self->state;
}

auto miral::WindowSpecification::preferred_orientation() const -> mir::optional_value<MirOrientationMode> const&
{
    return self->preferred_orientation;
}

auto miral::WindowSpecification::aux_rect() const -> mir::optional_value<Rectangle> const&
{
    return self->aux_rect;
}

auto miral::WindowSpecification::placement_hints() const -> mir::optional_value<MirPlacementHints> const&
{
    return self->placement_hints;
}

auto miral::WindowSpecification::window_placement_gravity() const -> mir::optional_value<MirPlacementGravity> const&
{
    return self->window_placement_gravity;
}

auto miral::WindowSpecification::aux_rect_placement_gravity() const -> mir::optional_value<MirPlacementGravity> const&
{
    return self->aux_rect_placement_gravity;
}

auto miral::WindowSpecification::aux_rect_placement_offset() const -> mir::optional_value<Displacement> const&
{
    return self->aux_rect_placement_offset;
}

auto miral::WindowSpecification::min_width() const -> mir::optional_value<Width> const&
{
    return self->min_width;
}

auto miral::WindowSpecification::min_height() const -> mir::optional_value<Height> const&
{
    return self->min_height;
}

auto miral::WindowSpecification::max_width() const -> mir::optional_value<Width> const&
{
    return self->max_width;
}

auto miral::WindowSpecification::max_height() const -> mir::optional_value<Height> const&
{
    return self->max_height;
}

auto miral::WindowSpecification::width_inc() const -> mir::optional_value<DeltaX> const&
{
    return self->width_inc;
}

auto miral::WindowSpecification::height_inc() const -> mir::optional_value<DeltaY> const&
{
    return self->height_inc;
}

auto miral::WindowSpecification::min_aspect() const -> mir::optional_value<AspectRatio> const&
{
    return self->min_aspect;
}

auto miral::WindowSpecification::max_aspect() const -> mir::optional_value<AspectRatio> const&
{
    return self->max_aspect;
}

auto miral::WindowSpecification::parent() const -> mir::optional_value<std::weak_ptr<mir::scene::Surface>> const&
{
    return self->parent;
}

auto miral::WindowSpecification::input_shape() const -> mir::optional_value<std::vector<Rectangle>> const&
{
    return self->input_shape;
}

auto miral::WindowSpecification::input_mode() const -> mir::optional_value<InputReceptionMode> const&
{
    return self->input_mode;
}

auto miral::WindowSpecification::shell_chrome() const -> mir::optional_value<MirShellChrome> const&
{
    return self->shell_chrome;
}

auto miral::WindowSpecification::confine_pointer() const -> mir::optional_value<MirPointerConfinementState> const&
{
    return self->confine_pointer;
}

auto miral::WindowSpecification::depth_layer() const -> mir::optional_value<MirDepthLayer> const&
{
    return self->depth_layer;
}

auto miral::WindowSpecification::attached_edges() const -> mir::optional_value<MirPlacementGravity> const&
{
    return self->attached_edges;
}

auto miral::WindowSpecification::exclusive_rect() const
    -> mir::optional_value<mir::optional_value<mir::geometry::Rectangle>> const&
{
    return self->exclusive_rect;
}

auto miral::WindowSpecification::ignore_exclusion_zones() const
    -> mir::optional_value<bool> const&
{
    return self->ignore_exclusion_zones;
}

auto miral::WindowSpecification::userdata() const -> mir::optional_value<std::shared_ptr<void>> const&
{
    return self->userdata;
}

auto miral::WindowSpecification::top_left() -> mir::optional_value<Point>&
{
    return self->top_left;
}

auto miral::WindowSpecification::size() -> mir::optional_value<Size>&
{
    return self->size;
}

auto miral::WindowSpecification::name() -> mir::optional_value<std::string>&
{
    return self->name;
}

auto miral::WindowSpecification::output_id() -> mir::optional_value<int>&
{
    return self->output_id;
}

auto miral::WindowSpecification::type() -> mir::optional_value<MirWindowType>&
{
    return self->type;
}

auto miral::WindowSpecification::state() -> mir::optional_value<MirWindowState>&
{
    return self->state;
}

auto miral::WindowSpecification::preferred_orientation() -> mir::optional_value<MirOrientationMode>&
{
    return self->preferred_orientation;
}

auto miral::WindowSpecification::aux_rect() -> mir::optional_value<Rectangle>&
{
    return self->aux_rect;
}

auto miral::WindowSpecification::placement_hints() -> mir::optional_value<MirPlacementHints>&
{
    return self->placement_hints;
}

auto miral::WindowSpecification::window_placement_gravity() -> mir::optional_value<MirPlacementGravity>&
{
    return self->window_placement_gravity;
}

auto miral::WindowSpecification::aux_rect_placement_gravity() -> mir::optional_value<MirPlacementGravity>&
{
    return self->aux_rect_placement_gravity;
}

auto miral::WindowSpecification::aux_rect_placement_offset() -> mir::optional_value<Displacement>&
{
    return self->aux_rect_placement_offset;
}

auto miral::WindowSpecification::min_width() -> mir::optional_value<Width>&
{
    return self->min_width;
}

auto miral::WindowSpecification::min_height() -> mir::optional_value<Height>&
{
    return self->min_height;
}

auto miral::WindowSpecification::max_width() -> mir::optional_value<Width>&
{
    return self->max_width;
}

auto miral::WindowSpecification::max_height() -> mir::optional_value<Height>&
{
    return self->max_height;
}

auto miral::WindowSpecification::width_inc() -> mir::optional_value<DeltaX>&
{
    return self->width_inc;
}

auto miral::WindowSpecification::height_inc() -> mir::optional_value<DeltaY>&
{
    return self->height_inc;
}

auto miral::WindowSpecification::min_aspect() -> mir::optional_value<AspectRatio>&
{
    return self->min_aspect;
}

auto miral::WindowSpecification::max_aspect() -> mir::optional_value<AspectRatio>&
{
    return self->max_aspect;
}

auto miral::WindowSpecification::parent() -> mir::optional_value<std::weak_ptr<mir::scene::Surface>>&
{
    return self->parent;
}

auto miral::WindowSpecification::input_shape() -> mir::optional_value<std::vector<Rectangle>>&
{
    return self->input_shape;
}

auto miral::WindowSpecification::input_mode() -> mir::optional_value<InputReceptionMode>&
{
    return self->input_mode;
}

auto miral::WindowSpecification::shell_chrome() -> mir::optional_value<MirShellChrome>&
{
    return self->shell_chrome;
}

auto miral::WindowSpecification::confine_pointer() -> mir::optional_value<MirPointerConfinementState>&
{
    return self->confine_pointer;
}

auto miral::WindowSpecification::depth_layer() -> mir::optional_value<MirDepthLayer>&
{
    return self->depth_layer;
}

auto miral::WindowSpecification::attached_edges() -> mir::optional_value<MirPlacementGravity>&
{
    return self->attached_edges;
}

auto miral::WindowSpecification::exclusive_rect()
    -> mir::optional_value<mir::optional_value<mir::geometry::Rectangle>>&
{
    return self->exclusive_rect;
}

auto miral::WindowSpecification::ignore_exclusion_zones()
    -> mir::optional_value<bool>&
{
    return self->ignore_exclusion_zones;
}

auto miral::WindowSpecification::application_id() const -> mir::optional_value<std::string> const&
{
    return self->application_id;
}

auto miral::WindowSpecification::application_id() -> mir::optional_value<std::string>&
{
    return self->application_id;
}

auto miral::WindowSpecification::server_side_decorated() const -> mir::optional_value<bool> const&
{
    return self->server_side_decorated;
}

auto miral::WindowSpecification::server_side_decorated() -> mir::optional_value<bool>&
{
    return self->server_side_decorated;
}

auto miral::WindowSpecification::focus_mode() const -> mir::optional_value<MirFocusMode> const&
{
    return self->focus_mode;
}

auto miral::WindowSpecification::focus_mode() -> mir::optional_value<MirFocusMode>&
{
    return self->focus_mode;
}

auto miral::WindowSpecification::visible_on_lock_screen() const -> mir::optional_value<bool> const&
{
    return self->visible_on_lock_screen;
}

auto miral::WindowSpecification::visible_on_lock_screen() -> mir::optional_value<bool>&
{
    return self->visible_on_lock_screen;
}

auto miral::WindowSpecification::tiled_edges() const -> mir::optional_value<mir::Flags<MirTiledEdge>> const&
{
    return self->tiled_edges;
}

auto miral::WindowSpecification::tiled_edges() -> mir::optional_value<mir::Flags<MirTiledEdge>>&
{
    return self->tiled_edges;
}

auto miral::WindowSpecification::userdata() -> mir::optional_value<std::shared_ptr<void>>&
{
    return self->userdata;
}

auto miral::WindowSpecification::to_surface_specification() const -> mir::shell::SurfaceSpecification
{
    return make_surface_spec(*this);
}
