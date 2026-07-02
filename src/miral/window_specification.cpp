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

#include <miral/window_specification.h>
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
    input_mode(spec.input_mode.transform([](auto v) { return static_cast<InputReceptionMode>(v); })),
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
    tiled_edges(spec.tiled_edges),
    opacity(spec.alpha),
    parent_size(spec.parent_size)
{
    if (spec.aux_rect_placement_offset_x.has_value() && spec.aux_rect_placement_offset_y.has_value())
        aux_rect_placement_offset = Displacement{spec.aux_rect_placement_offset_x.value(), spec.aux_rect_placement_offset_y.value()};

    if (spec.edge_attachment.has_value() && !placement_hints.has_value())
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            placement_hints = MirPlacementHints(mir_placement_hints_flip_any | mir_placement_hints_antipodes);
#pragma GCC diagnostic pop
            break;
        }
    }

    if (spec.width.has_value() && spec.height.has_value())
        size = Size(spec.width.value(), spec.height.value());

    if (spec.buffer_usage.has_value())
        buffer_usage = BufferUsage(spec.buffer_usage.value());

    if (spec.output_id.has_value())
        output_id = spec.output_id.value().as_value();

    if (spec.min_aspect.has_value())
        min_aspect = AspectRatio{spec.min_aspect.value().width, spec.min_aspect.value().height};

    if (spec.max_aspect.has_value())
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

auto miral::WindowSpecification::top_left() const -> std::optional<Point> const&
{
    return self->top_left;
}

auto miral::WindowSpecification::size() const -> std::optional<Size> const&
{
    return self->size;
}

auto miral::WindowSpecification::name() const -> std::optional<std::string> const&
{
    return self->name;
}

auto miral::WindowSpecification::output_id() const -> std::optional<int> const&
{
    return self->output_id;
}

auto miral::WindowSpecification::type() const -> std::optional<MirWindowType> const&
{
    return self->type;
}

auto miral::WindowSpecification::state() const -> std::optional<MirWindowState> const&
{
    return self->state;
}

auto miral::WindowSpecification::preferred_orientation() const -> std::optional<MirOrientationMode> const&
{
    return self->preferred_orientation;
}

auto miral::WindowSpecification::aux_rect() const -> std::optional<Rectangle> const&
{
    return self->aux_rect;
}

auto miral::WindowSpecification::placement_hints() const -> std::optional<MirPlacementHints> const&
{
    return self->placement_hints;
}

auto miral::WindowSpecification::window_placement_gravity() const -> std::optional<MirPlacementGravity> const&
{
    return self->window_placement_gravity;
}

auto miral::WindowSpecification::aux_rect_placement_gravity() const -> std::optional<MirPlacementGravity> const&
{
    return self->aux_rect_placement_gravity;
}

auto miral::WindowSpecification::aux_rect_placement_offset() const -> std::optional<Displacement> const&
{
    return self->aux_rect_placement_offset;
}

auto miral::WindowSpecification::min_width() const -> std::optional<Width> const&
{
    return self->min_width;
}

auto miral::WindowSpecification::min_height() const -> std::optional<Height> const&
{
    return self->min_height;
}

auto miral::WindowSpecification::max_width() const -> std::optional<Width> const&
{
    return self->max_width;
}

auto miral::WindowSpecification::max_height() const -> std::optional<Height> const&
{
    return self->max_height;
}

auto miral::WindowSpecification::width_inc() const -> std::optional<DeltaX> const&
{
    return self->width_inc;
}

auto miral::WindowSpecification::height_inc() const -> std::optional<DeltaY> const&
{
    return self->height_inc;
}

auto miral::WindowSpecification::min_aspect() const -> std::optional<AspectRatio> const&
{
    return self->min_aspect;
}

auto miral::WindowSpecification::max_aspect() const -> std::optional<AspectRatio> const&
{
    return self->max_aspect;
}

auto miral::WindowSpecification::parent() const -> std::optional<std::weak_ptr<mir::scene::Surface>> const&
{
    return self->parent;
}

auto miral::WindowSpecification::input_shape() const -> std::optional<std::vector<Rectangle>> const&
{
    return self->input_shape;
}

auto miral::WindowSpecification::input_mode() const -> std::optional<InputReceptionMode> const&
{
    return self->input_mode;
}

auto miral::WindowSpecification::shell_chrome() const -> std::optional<MirShellChrome> const&
{
    return self->shell_chrome;
}

auto miral::WindowSpecification::confine_pointer() const -> std::optional<MirPointerConfinementState> const&
{
    return self->confine_pointer;
}

auto miral::WindowSpecification::depth_layer() const -> std::optional<MirDepthLayer> const&
{
    return self->depth_layer;
}

auto miral::WindowSpecification::attached_edges() const -> std::optional<MirPlacementGravity> const&
{
    return self->attached_edges;
}

auto miral::WindowSpecification::exclusive_rect() const
    -> std::optional<std::optional<mir::geometry::Rectangle>> const&
{
    return self->exclusive_rect;
}

auto miral::WindowSpecification::ignore_exclusion_zones() const
    -> std::optional<bool> const&
{
    return self->ignore_exclusion_zones;
}

auto miral::WindowSpecification::userdata() const -> std::optional<std::shared_ptr<void>> const&
{
    return self->userdata;
}

auto miral::WindowSpecification::top_left() -> std::optional<Point>&
{
    return self->top_left;
}

auto miral::WindowSpecification::size() -> std::optional<Size>&
{
    return self->size;
}

auto miral::WindowSpecification::name() -> std::optional<std::string>&
{
    return self->name;
}

auto miral::WindowSpecification::output_id() -> std::optional<int>&
{
    return self->output_id;
}

auto miral::WindowSpecification::type() -> std::optional<MirWindowType>&
{
    return self->type;
}

auto miral::WindowSpecification::state() -> std::optional<MirWindowState>&
{
    return self->state;
}

auto miral::WindowSpecification::preferred_orientation() -> std::optional<MirOrientationMode>&
{
    return self->preferred_orientation;
}

auto miral::WindowSpecification::aux_rect() -> std::optional<Rectangle>&
{
    return self->aux_rect;
}

auto miral::WindowSpecification::placement_hints() -> std::optional<MirPlacementHints>&
{
    return self->placement_hints;
}

auto miral::WindowSpecification::window_placement_gravity() -> std::optional<MirPlacementGravity>&
{
    return self->window_placement_gravity;
}

auto miral::WindowSpecification::aux_rect_placement_gravity() -> std::optional<MirPlacementGravity>&
{
    return self->aux_rect_placement_gravity;
}

auto miral::WindowSpecification::aux_rect_placement_offset() -> std::optional<Displacement>&
{
    return self->aux_rect_placement_offset;
}

auto miral::WindowSpecification::min_width() -> std::optional<Width>&
{
    return self->min_width;
}

auto miral::WindowSpecification::min_height() -> std::optional<Height>&
{
    return self->min_height;
}

auto miral::WindowSpecification::max_width() -> std::optional<Width>&
{
    return self->max_width;
}

auto miral::WindowSpecification::max_height() -> std::optional<Height>&
{
    return self->max_height;
}

auto miral::WindowSpecification::width_inc() -> std::optional<DeltaX>&
{
    return self->width_inc;
}

auto miral::WindowSpecification::height_inc() -> std::optional<DeltaY>&
{
    return self->height_inc;
}

auto miral::WindowSpecification::min_aspect() -> std::optional<AspectRatio>&
{
    return self->min_aspect;
}

auto miral::WindowSpecification::max_aspect() -> std::optional<AspectRatio>&
{
    return self->max_aspect;
}

auto miral::WindowSpecification::parent() -> std::optional<std::weak_ptr<mir::scene::Surface>>&
{
    return self->parent;
}

auto miral::WindowSpecification::input_shape() -> std::optional<std::vector<Rectangle>>&
{
    return self->input_shape;
}

auto miral::WindowSpecification::input_mode() -> std::optional<InputReceptionMode>&
{
    return self->input_mode;
}

auto miral::WindowSpecification::shell_chrome() -> std::optional<MirShellChrome>&
{
    return self->shell_chrome;
}

auto miral::WindowSpecification::confine_pointer() -> std::optional<MirPointerConfinementState>&
{
    return self->confine_pointer;
}

auto miral::WindowSpecification::depth_layer() -> std::optional<MirDepthLayer>&
{
    return self->depth_layer;
}

auto miral::WindowSpecification::attached_edges() -> std::optional<MirPlacementGravity>&
{
    return self->attached_edges;
}

auto miral::WindowSpecification::exclusive_rect()
    -> std::optional<std::optional<mir::geometry::Rectangle>>&
{
    return self->exclusive_rect;
}

auto miral::WindowSpecification::ignore_exclusion_zones()
    -> std::optional<bool>&
{
    return self->ignore_exclusion_zones;
}

auto miral::WindowSpecification::application_id() const -> std::optional<std::string> const&
{
    return self->application_id;
}

auto miral::WindowSpecification::application_id() -> std::optional<std::string>&
{
    return self->application_id;
}

auto miral::WindowSpecification::server_side_decorated() const -> std::optional<bool> const&
{
    return self->server_side_decorated;
}

auto miral::WindowSpecification::server_side_decorated() -> std::optional<bool>&
{
    return self->server_side_decorated;
}

auto miral::WindowSpecification::focus_mode() const -> std::optional<MirFocusMode> const&
{
    return self->focus_mode;
}

auto miral::WindowSpecification::focus_mode() -> std::optional<MirFocusMode>&
{
    return self->focus_mode;
}

auto miral::WindowSpecification::visible_on_lock_screen() const -> std::optional<bool> const&
{
    return self->visible_on_lock_screen;
}

auto miral::WindowSpecification::visible_on_lock_screen() -> std::optional<bool>&
{
    return self->visible_on_lock_screen;
}

auto miral::WindowSpecification::tiled_edges() const -> std::optional<mir::Flags<MirTiledEdge>> const&
{
    return self->tiled_edges;
}

auto miral::WindowSpecification::tiled_edges() -> std::optional<mir::Flags<MirTiledEdge>>&
{
    return self->tiled_edges;
}

auto miral::WindowSpecification::alpha() -> std::optional<float>&
{
    return self->opacity;
}

auto miral::WindowSpecification::alpha() const -> std::optional<float> const&
{
    return self->opacity;
}

auto miral::WindowSpecification::parent_size() -> std::optional<Size>&
{
    return self->parent_size;
}

auto miral::WindowSpecification::parent_size() const -> std::optional<Size> const&
{
    return self->parent_size;
}

auto miral::WindowSpecification::userdata() -> std::optional<std::shared_ptr<void>>&
{
    return self->userdata;
}

auto miral::WindowSpecification::to_surface_specification() const -> mir::shell::SurfaceSpecification
{
    return make_surface_spec(*this);
}
