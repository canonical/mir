/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "miral/window_specification.h"

#include <mir/shell/surface_specification.h>
#include <mir/scene/surface_creation_parameters.h>

struct miral::WindowSpecification::Self
{
    Self() = default;
    Self(Self const&) = default;
    Self(mir::shell::SurfaceSpecification const& spec);
    Self(mir::scene::SurfaceCreationParameters const& params);

    void update(mir::scene::SurfaceCreationParameters& params) const;

    mir::optional_value<Point> top_left;
    mir::optional_value<Size> size;
    mir::optional_value<MirPixelFormat> pixel_format;
    mir::optional_value<BufferUsage> buffer_usage;
    mir::optional_value<std::string> name;
    mir::optional_value<int> output_id;
    mir::optional_value<MirWindowType> type;
    mir::optional_value<MirWindowState> state;
    mir::optional_value<MirOrientationMode> preferred_orientation;
    std::weak_ptr<mir::frontend::BufferStream> content;
    mir::optional_value<Rectangle> aux_rect;
    mir::optional_value<MirPlacementHints> placement_hints;
    mir::optional_value<MirPlacementGravity> window_placement_gravity;
    mir::optional_value<MirPlacementGravity> aux_rect_placement_gravity;
    mir::optional_value<Displacement> aux_rect_placement_offset;
    mir::optional_value<Width> min_width;
    mir::optional_value<Height> min_height;
    mir::optional_value<Width> max_width;
    mir::optional_value<Height> max_height;
    mir::optional_value<DeltaX> width_inc;
    mir::optional_value<DeltaY> height_inc;
    mir::optional_value<AspectRatio> min_aspect;
    mir::optional_value<AspectRatio> max_aspect;
    mir::optional_value<std::vector<mir::shell::StreamSpecification>> streams;
    mir::optional_value<std::weak_ptr<mir::scene::Surface>> parent;
    mir::optional_value<std::vector<Rectangle>> input_shape;
    mir::optional_value<InputReceptionMode> input_mode;
    mir::optional_value<MirShellChrome> shell_chrome;
    mir::optional_value<MirPointerConfinementState> confine_pointer;
    mir::optional_value<MirDepthLayer> depth_layer;
    mir::optional_value<MirPlacementGravity> attached_edges;
    mir::optional_value<mir::optional_value<mir::geometry::Rectangle>> exclusive_rect;
    mir::optional_value<std::string> application_id;
    mir::optional_value<bool> server_side_decorated;
    mir::optional_value<std::shared_ptr<void>> userdata;
};

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
    input_mode(),
    shell_chrome(spec.shell_chrome),
    confine_pointer(spec.confine_pointer),
    depth_layer(spec.depth_layer),
    attached_edges(spec.attached_edges),
    exclusive_rect(spec.exclusive_rect),
    application_id(spec.application_id),
    server_side_decorated() // Not currently on SurfaceSpecification
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

namespace
{
template<typename Dest, typename Source>
void copy_if_set(Dest& dest, mir::optional_value<Source> const& source)
{
    if (source.is_set()) dest = source.value();
}

template<typename Dest, typename Source>
void copy_if_set(Dest& dest, std::weak_ptr<Source> const& source)
{
    if (source.lock()) dest = source;
}

template<typename Dest, typename Source>
void copy_if_set(mir::optional_value<Dest>& dest, mir::optional_value<Source> const& source)
{
    if (source.is_set()) dest = source;
}

template<typename Source>
void copy_if_set(mir::graphics::BufferUsage& dest, mir::optional_value<Source> const& source)
{
    if (source.is_set()) dest = static_cast<mir::graphics::BufferUsage>(source.value());
}

template<typename Source>
void copy_if_set(
    mir::graphics::DisplayConfigurationOutputId& dest,
    mir::optional_value<Source> const& source)
{
    if (source.is_set()) dest = static_cast<mir::graphics::DisplayConfigurationOutputId>(source.value());
}

template<typename Source>
void copy_if_set(
    mir::input::InputReceptionMode& dest,
    mir::optional_value<Source> const& source)
{
    if (source.is_set()) dest = static_cast<mir::input::InputReceptionMode>(source.value());
}

template<typename Source>
void copy_if_set(
    mir::optional_value<mir::shell::SurfaceAspectRatio>& dest,
    mir::optional_value<Source> const& source)
{
    if (source.is_set()) dest = mir::shell::SurfaceAspectRatio{source.value().width, source.value().height};
}

template<typename Source>
void copy_if_set(
    mir::optional_value<mir::frontend::BufferStreamId>& dest,
    mir::optional_value<Source> const& source)
{
    if (source.is_set()) dest = mir::frontend::BufferStreamId{source.value().as_value()};
}
}

miral::WindowSpecification::Self::Self(mir::scene::SurfaceCreationParameters const& params) :
    top_left(params.top_left),
    size(params.size),
    pixel_format(params.pixel_format),
    buffer_usage(static_cast<BufferUsage>(params.buffer_usage)),
    name(params.name),
    output_id(params.output_id.as_value()),
    type(params.type),
    state(params.state),
    preferred_orientation(params.preferred_orientation),
    content(params.content),
    aux_rect(params.aux_rect),
    placement_hints(params.placement_hints),
    window_placement_gravity(params.surface_placement_gravity),
    aux_rect_placement_gravity(params.aux_rect_placement_gravity),
    min_width(params.min_width),
    min_height(params.min_height),
    max_width(params.max_width),
    max_height(params.max_height),
    width_inc(params.width_inc),
    height_inc(params.height_inc),
    min_aspect(),
    max_aspect(),
    streams(params.streams),
    parent(params.parent),
    input_shape(params.input_shape),
    input_mode(static_cast<InputReceptionMode>(params.input_mode)),
    shell_chrome(params.shell_chrome),
    confine_pointer(params.confine_pointer),
    depth_layer(params.depth_layer),
    attached_edges(params.attached_edges),
    exclusive_rect(params.exclusive_rect),
    application_id(params.application_id),
    server_side_decorated(params.server_side_decorated)
{
    if (params.aux_rect_placement_offset_x.is_set() && params.aux_rect_placement_offset_y.is_set())
        aux_rect_placement_offset = Displacement{params.aux_rect_placement_offset_x.value(), params.aux_rect_placement_offset_y.value()};

    if (params.edge_attachment.is_set() && !placement_hints.is_set())
    {
        switch (params.edge_attachment.value())
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
            placement_hints = mir_placement_hints_flip_any;
            break;
        }
    }

    if (params.min_aspect.is_set())
        min_aspect = AspectRatio{params.min_aspect.value().width, params.min_aspect.value().height};

    if (params.max_aspect.is_set())
        max_aspect = AspectRatio{params.max_aspect.value().width, params.max_aspect.value().height};
}

void miral::WindowSpecification::Self::update(mir::scene::SurfaceCreationParameters& params) const
{
    copy_if_set(params.top_left, top_left);
    copy_if_set(params.size, size);
    copy_if_set(params.pixel_format, pixel_format);
    copy_if_set(params.buffer_usage, buffer_usage);
    copy_if_set(params.name, name);
    copy_if_set(params.output_id, output_id);
    copy_if_set(params.type, type);
    copy_if_set(params.state, state);
    copy_if_set(params.preferred_orientation, preferred_orientation);
    copy_if_set(params.content, content);
    copy_if_set(params.aux_rect, aux_rect);
    copy_if_set(params.min_width, min_width);
    copy_if_set(params.min_height, min_height);
    copy_if_set(params.max_width, max_width);
    copy_if_set(params.max_height, max_height);
    copy_if_set(params.width_inc, width_inc);
    copy_if_set(params.height_inc, height_inc);
    copy_if_set(params.min_aspect, min_aspect);
    copy_if_set(params.max_aspect, max_aspect);
    copy_if_set(params.streams, streams);
    copy_if_set(params.parent, parent);
    copy_if_set(params.input_shape, input_shape);
    copy_if_set(params.input_mode, input_mode);
    copy_if_set(params.shell_chrome, shell_chrome);
    copy_if_set(params.confine_pointer, confine_pointer);
    copy_if_set(params.placement_hints, placement_hints);
    copy_if_set(params.surface_placement_gravity, window_placement_gravity);
    copy_if_set(params.aux_rect_placement_gravity, aux_rect_placement_gravity);
    copy_if_set(params.depth_layer, depth_layer);
    copy_if_set(params.attached_edges, attached_edges);
    copy_if_set(params.exclusive_rect, exclusive_rect.value());
    copy_if_set(params.application_id, application_id);
    copy_if_set(params.server_side_decorated, server_side_decorated);

    if (aux_rect_placement_offset.is_set())
    {
        auto const offset = aux_rect_placement_offset.value();
        params.aux_rect_placement_offset_x = offset.dx.as_int();
        params.aux_rect_placement_offset_y = offset.dy.as_int();
    }
}

miral::WindowSpecification::WindowSpecification() :
    self{std::make_unique<Self>()}
{
}

miral::WindowSpecification::WindowSpecification(mir::shell::SurfaceSpecification const& spec) :
    self{std::make_unique<Self>(spec)}
{
}

miral::WindowSpecification::WindowSpecification(mir::scene::SurfaceCreationParameters const& params) :
    self{std::make_unique<Self>(params)}
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

void miral::WindowSpecification::update(mir::scene::SurfaceCreationParameters& params) const
{
    self->update(params);
}

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

auto miral::WindowSpecification::userdata() -> mir::optional_value<std::shared_ptr<void>>&
{
    return self->userdata;
}
