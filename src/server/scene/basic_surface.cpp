/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/scene/basic_surface.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/frontend/event_sink.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/cursor_image.h"
#include "mir/graphics/null_display_configuration_observer.h"
#include "mir/geometry/displacement.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/observer_multiplexer.h"
#include "mir/scene/surface_observer.h"

#include "mir/scene/scene_report.h"

#include <boost/throw_exception.hpp>

#include <algorithm>
#include <cstring>
#include <latch>
#include <stdexcept>

namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace geom = mir::geometry;
namespace mrs = mir::renderer::software;

class ms::BasicSurface::DisplayConfigurationEarlyListener :  public mg::NullDisplayConfigurationObserver
{
public:
    explicit DisplayConfigurationEarlyListener(BasicSurface* surface) : surface{surface} {}

private:
    void initial_configuration(std::shared_ptr<mg::DisplayConfiguration const> const& config) override
    {
        surface->display_config = config;
    }
    void configuration_applied(std::shared_ptr<mg::DisplayConfiguration const> const& config) override
    {
        surface->display_config = config;
        surface->track_outputs();
    }

    BasicSurface* surface;
};

class ms::BasicSurface::Multiplexer : public ObserverMultiplexer<SurfaceObserver>
{
public:
    Multiplexer()
        : ObserverMultiplexer{linearising_executor}
    {
    }

    void attrib_changed(Surface const* surf, MirWindowAttrib attrib, int value) override
    {
        for_each_observer(&SurfaceObserver::attrib_changed, surf, attrib, value);
    }

    void window_resized_to(Surface const* surf, geometry::Size const& window_size) override
    {
        for_each_observer(&SurfaceObserver::window_resized_to, surf, window_size);
    }

    void content_resized_to(Surface const* surf, geometry::Size const& content_size) override
    {
        for_each_observer(&SurfaceObserver::content_resized_to, surf, content_size);
    }

    void moved_to(Surface const* surf, geometry::Point const& top_left) override
    {
        for_each_observer(&SurfaceObserver::moved_to, surf, top_left);
    }

    void hidden_set_to(Surface const* surf, bool hide) override
    {
        for_each_observer(&SurfaceObserver::hidden_set_to, surf, hide);
    }

    void frame_posted(Surface const* surf, geometry::Rectangle const& damage) override
    {
        for_each_observer(&SurfaceObserver::frame_posted, surf, damage);
    }

    void alpha_set_to(Surface const* surf, float alpha) override
    {
        for_each_observer(&SurfaceObserver::alpha_set_to, surf, alpha);
    }

    void orientation_set_to(Surface const* surf, MirOrientation orientation) override
    {
        for_each_observer(&SurfaceObserver::orientation_set_to, surf, orientation);
    }

    void transformation_set_to(Surface const* surf, glm::mat4 const& t) override
    {
        for_each_observer(&SurfaceObserver::transformation_set_to, surf, t);
    }

    void reception_mode_set_to(Surface const* surf, input::InputReceptionMode mode) override
    {
        for_each_observer(&SurfaceObserver::reception_mode_set_to, surf, mode);
    }

    void cursor_image_set_to(Surface const* surf, std::weak_ptr<graphics::CursorImage> const& image) override
    {
        for_each_observer(&SurfaceObserver::cursor_image_set_to, surf, image);
    }

    void client_surface_close_requested(Surface const* surf) override
    {
        for_each_observer(&SurfaceObserver::client_surface_close_requested, surf);
    }

    void renamed(Surface const* surf, std::string const& name) override
    {
        for_each_observer(&SurfaceObserver::renamed, surf, name);
    }

    void cursor_image_removed(Surface const* surf) override
    {
        for_each_observer(&SurfaceObserver::cursor_image_removed, surf);
    }

    void placed_relative(Surface const* surf, geometry::Rectangle const& placement) override
    {
        for_each_observer(&SurfaceObserver::placed_relative, surf, placement);
    }

    void input_consumed(Surface const* surf, std::shared_ptr<MirEvent const> const& event) override
    {
        for_each_observer(&SurfaceObserver::input_consumed, surf, event);
    }

    void depth_layer_set_to(Surface const* surf, MirDepthLayer depth_layer) override
    {
        for_each_observer(&SurfaceObserver::depth_layer_set_to, surf, depth_layer);
    }

    void application_id_set_to(Surface const* surf, std::string const& application_id) override
    {
        for_each_observer(&SurfaceObserver::application_id_set_to, surf, application_id);
    }

    void entered_output(Surface const* surf, graphics::DisplayConfigurationOutputId const& id) override
    {
        for_each_observer(&SurfaceObserver::entered_output, surf, id);
    }

    void left_output(Surface const* surf, graphics::DisplayConfigurationOutputId const& id) override
    {
        for_each_observer(&SurfaceObserver::left_output, surf, id);
    }

    void rescale_output(Surface const* surf, graphics::DisplayConfigurationOutputId const& id) override
    {
        for_each_observer(&SurfaceObserver::rescale_output, surf, id);
    }

    void tiled_edges(Surface const* surf, Flags<MirTiledEdge> edges) override
    {
        for_each_observer(&SurfaceObserver::tiled_edges, surf, edges);
    }
};

ms::BasicSurface::BasicSurface(
    std::string const& name,
    geometry::Rectangle rect,
    std::weak_ptr<Surface> const& parent,
    MirPointerConfinementState confinement_state,
    std::list<StreamInfo> const& layers,
    std::shared_ptr<mg::CursorImage> const& cursor_image,
    std::shared_ptr<SceneReport> const& report,
    std::shared_ptr<ObserverRegistrar<mg::DisplayConfigurationObserver>> const& display_config_registrar) :
    synchronised_state{
        State {
            .surface_name = name,
            .surface_rect = rect,
            .transformation_matrix = glm::mat4{1},
            .surface_alpha = 1.0f,
            .hidden = false,
            .input_mode = mi::InputReceptionMode::normal,
            .cursor_image{cursor_image},
            .layers{layers},
            .confine_pointer_state = confinement_state,
        }
    },
    observers(std::make_shared<Multiplexer>()),
    report(report),
    parent_(parent),
    display_config_registrar{display_config_registrar},
    display_config_monitor{std::make_shared<DisplayConfigurationEarlyListener>(this)}
{
    auto state = synchronised_state.lock();
    update_frame_posted_callbacks(*state);
    report->surface_created(this, state->surface_name);
    display_config_registrar->register_early_observer(display_config_monitor, immediate_executor);
}

ms::BasicSurface::BasicSurface(
    std::string const& name,
    geometry::Rectangle rect,
    MirPointerConfinementState state,
    std::list<StreamInfo> const& layers,
    std::shared_ptr<mg::CursorImage> const& cursor_image,
    std::shared_ptr<SceneReport> const& report,
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& display_config_registrar) :
    BasicSurface(
        name,
        rect,
        std::shared_ptr<Surface>{nullptr},
        state,
        layers,
        cursor_image,
        report,
        display_config_registrar)
{
}

ms::BasicSurface::~BasicSurface() noexcept
{
    auto state = synchronised_state.lock();
    clear_frame_posted_callbacks(*state);
    report->surface_deleted(this, state->surface_name);
    display_config_registrar->unregister_interest(*display_config_monitor);
}

void ms::BasicSurface::register_interest(std::weak_ptr<SurfaceObserver> const& observer)
{
    observers->register_interest(observer);
}

void ms::BasicSurface::register_interest(std::weak_ptr<SurfaceObserver> const& observer, Executor& executor)
{
    observers->register_interest(observer, executor);
}

void ms::BasicSurface::register_early_observer(std::weak_ptr<SurfaceObserver> const& observer, Executor& executor)
{
    observers->register_early_observer(observer, executor);
}

void ms::BasicSurface::unregister_interest(SurfaceObserver const& observer)
{
    observers->unregister_interest(observer);
}

void ms::BasicSurface::initial_placement_done()
{
    linearised_track_outputs();
}

std::string ms::BasicSurface::name() const
{
    return synchronised_state.lock()->surface_name;
}

void ms::BasicSurface::move_to(geometry::Point const& top_left)
{
    synchronised_state.lock()->surface_rect.top_left = top_left;
    observers->moved_to(this, top_left);
    linearised_track_outputs();
}

void ms::BasicSurface::set_hidden(bool hide)
{
    synchronised_state.lock()->hidden = hide;
    observers->hidden_set_to(this, hide);
}

mir::geometry::Size ms::BasicSurface::window_size() const
{
    return synchronised_state.lock()->surface_rect.size;
}

mir::geometry::Displacement ms::BasicSurface::content_offset() const
{
    auto state = synchronised_state.lock();
    return geom::Displacement{state->margins.left, state->margins.top};
}

mir::geometry::Size ms::BasicSurface::content_size() const
{
    return content_size(*synchronised_state.lock());
}

void ms::BasicSurface::set_input_region(std::vector<geom::Rectangle> const& input_rectangles)
{
    synchronised_state.lock()->custom_input_rectangles = input_rectangles;
}

std::vector<geom::Rectangle> ms::BasicSurface::get_input_region() const
{
    auto state = synchronised_state.lock();
    return state->custom_input_rectangles;
}

void ms::BasicSurface::resize(geom::Size const& desired_size)
{
    geom::Size new_size = desired_size;
    if (new_size.width <= geom::Width{0})   new_size.width = geom::Width{1};
    if (new_size.height <= geom::Height{0}) new_size.height = geom::Height{1};

    auto state = synchronised_state.lock();
    if (new_size != state->surface_rect.size)
    {
        state->surface_rect.size = new_size;
        auto const content_size_ = content_size(*state);

        state.drop();

        observers->window_resized_to(this, new_size);
        observers->content_resized_to(this, content_size_);
        linearised_track_outputs();
    }
}

geom::Point ms::BasicSurface::top_left() const
{
    return synchronised_state.lock()->surface_rect.top_left;
}

geom::Rectangle ms::BasicSurface::input_bounds() const
{
    auto state = synchronised_state.lock();
    return geom::Rectangle{content_top_left(*state), content_size(*state)};
}

// TODO: Does not account for transformation().
bool ms::BasicSurface::input_area_contains(geom::Point const& point) const
{
    auto state = synchronised_state.lock();

    if (!visible(*state))
        return false;

    if (state->clip_area)
    {
        if (!state->clip_area.value().contains(point))
            return false;
    }

    if (state->custom_input_rectangles.empty())
    {
        // no custom input, restrict to bounding rectangle
        auto const input_rect = geom::Rectangle{content_top_left(*state), content_size(*state)};
        return input_rect.contains(point);
    }
    else
    {
        auto local_point = as_point(point - content_top_left(*state));
        for (auto const& rectangle : state->custom_input_rectangles)
        {
            if (rectangle.contains(local_point))
                return true;
        }
    }
    return false;
}

void ms::BasicSurface::set_alpha(float alpha)
{
    synchronised_state.lock()->surface_alpha = alpha;
    observers->alpha_set_to(this, alpha);
}

void ms::BasicSurface::set_orientation(MirOrientation orientation)
{
    observers->orientation_set_to(this, orientation);
}

void ms::BasicSurface::set_transformation(glm::mat4 const& t)
{
    synchronised_state.lock()->transformation_matrix = t;
    observers->transformation_set_to(this, t);
}

bool ms::BasicSurface::visible() const
{
    return visible(*synchronised_state.lock());
}

bool ms::BasicSurface::visible(State const& state) const
{
    bool visible{false};
    for (auto const& info : state.layers)
        visible |= info.stream->has_submitted_buffer();
    return !state.hidden && visible;
}

mi::InputReceptionMode ms::BasicSurface::reception_mode() const
{
    return synchronised_state.lock()->input_mode;
}

void ms::BasicSurface::set_reception_mode(mi::InputReceptionMode mode)
{
    synchronised_state.lock()->input_mode = mode;
    observers->reception_mode_set_to(this, mode);
}

MirWindowType ms::BasicSurface::type() const
{
    return synchronised_state.lock()->type;
}

MirWindowType ms::BasicSurface::set_type(MirWindowType t)
{
    if (t < mir_window_type_normal || t >= mir_window_types)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface "
            "type."));
    }

    auto state = synchronised_state.lock();
    if (state->type != t)
    {
        state->type = t;

        state.drop();

        observers->attrib_changed(this, mir_window_attrib_type, t);
    }

    return t;
}

MirWindowState ms::BasicSurface::state() const
{
    return synchronised_state.lock()->state.active_state();
}

auto ms::BasicSurface::state_tracker() const -> SurfaceStateTracker
{
    return synchronised_state.lock()->state;
}

MirWindowState ms::BasicSurface::set_state(MirWindowState s)
{
    if (s < mir_window_state_unknown || s >= mir_window_states)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface state."));

    auto state = synchronised_state.lock();
    if (state->state.active_state() != s)
    {
        state->state = state->state.with_active_state(s);

        state.drop();

        observers->attrib_changed(this, mir_window_attrib_state, s);
    }

    return s;
}

MirOrientationMode ms::BasicSurface::set_preferred_orientation(MirOrientationMode new_orientation_mode)
{
    if ((new_orientation_mode & mir_orientation_mode_any) == 0)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid orientation mode"));
    }

    auto state = synchronised_state.lock();
    if (state->pref_orientation_mode != new_orientation_mode)
    {
        state->pref_orientation_mode = new_orientation_mode;

        state.drop();
        observers->attrib_changed(this, mir_window_attrib_preferred_orientation, new_orientation_mode);
    }

    return new_orientation_mode;
}

int ms::BasicSurface::configure(MirWindowAttrib attrib, int value)
{
    int result = value;
    switch (attrib)
    {
    case mir_window_attrib_type:
        result = set_type(static_cast<MirWindowType>(result));
        break;
    case mir_window_attrib_state:
        result = set_state(static_cast<MirWindowState>(result));
        break;
    case mir_window_attrib_focus:
        set_focus_state(static_cast<MirWindowFocusState>(result));
        break;
    case mir_window_attrib_dpi:
        result = set_dpi(result);
        break;
    case mir_window_attrib_visibility:
        result = set_visibility(static_cast<MirWindowVisibility>(result));
        break;
    case mir_window_attrib_preferred_orientation:
        result = set_preferred_orientation(static_cast<MirOrientationMode>(result));
        break;
    default:
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface attribute."));
    }

    return result;
}

int ms::BasicSurface::query(MirWindowAttrib attrib) const
{
    auto state = synchronised_state.lock();
    switch (attrib)
    {
        case mir_window_attrib_type: return state->type;
        case mir_window_attrib_state: return state->state.active_state();
        case mir_window_attrib_focus: return state->focus;
        case mir_window_attrib_dpi: return state->dpi;
        case mir_window_attrib_visibility: return state->visibility;
        case mir_window_attrib_preferred_orientation: return state->pref_orientation_mode;
        default: BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface "
                                                        "attribute."));
    }
}

void ms::BasicSurface::hide()
{
    set_hidden(true);
}

void ms::BasicSurface::show()
{
    set_hidden(false);
}

void ms::BasicSurface::set_cursor_image(std::shared_ptr<mg::CursorImage> const& image)
{
    synchronised_state.lock()->cursor_image = image;

    if (image)
        observers->cursor_image_set_to(this, image);
    else
        observers->cursor_image_removed(this);
}

void ms::BasicSurface::remove_cursor_image()
{
    synchronised_state.lock()->cursor_image = nullptr;
    observers->cursor_image_removed(this);
}

std::shared_ptr<mg::CursorImage> ms::BasicSurface::cursor_image() const
{
    return synchronised_state.lock()->cursor_image;
}

namespace
{
struct CursorImageFromBuffer : public mg::CursorImage
{
    CursorImageFromBuffer(
        std::shared_ptr<mg::Buffer> buffer,
        geom::Displacement const& hotspot)
        : buffer{mrs::as_read_mappable_buffer(std::move(buffer))},
          mapping{this->buffer->map_readable()},
          hotspot_(hotspot)
    {
    }
    void const* as_argb_8888() const
    {
        return mapping->data();
    }

    geom::Size size() const
    {
        return mapping->size();
    }

    geom::Displacement hotspot() const
    {
        return hotspot_;
    }

    std::shared_ptr<mrs::ReadMappableBuffer> const buffer;
    std::unique_ptr<mrs::Mapping<unsigned char const>> const mapping;
    geom::Displacement const hotspot_;
};
}

void ms::BasicSurface::set_cursor_from_buffer(
    std::shared_ptr<mg::Buffer> buffer,
    geom::Displacement const& hotspot)
{
    auto image = std::make_shared<CursorImageFromBuffer>(buffer, hotspot);
    synchronised_state.lock()->cursor_image = image;
    observers->cursor_image_set_to(this, image);
}

auto ms::BasicSurface::wayland_surface() -> mw::Weak<mf::WlSurface> const&
{
    return weak_surface;
}

void ms::BasicSurface::request_client_surface_close()
{
    observers->client_surface_close_requested(this);
}

int ms::BasicSurface::dpi() const
{
    return synchronised_state.lock()->dpi;
}

int ms::BasicSurface::set_dpi(int new_dpi)
{
    if (new_dpi < 0)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid DPI value"));
    }

    auto state = synchronised_state.lock();
    if (state->dpi != new_dpi)
    {
        state->dpi = new_dpi;

        state.drop();
        observers->attrib_changed(this, mir_window_attrib_dpi, new_dpi);
    }

    return new_dpi;
}

MirWindowVisibility ms::BasicSurface::set_visibility(MirWindowVisibility new_visibility)
{
    if (new_visibility != mir_window_visibility_occluded &&
        new_visibility != mir_window_visibility_exposed)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid visibility value"));
    }

    auto state = synchronised_state.lock();
    if (state->visibility != new_visibility)
    {
        state->visibility = new_visibility;

        state.drop();

        observers->attrib_changed(this, mir_window_attrib_visibility, new_visibility);
    }

    return new_visibility;
}

std::shared_ptr<ms::Surface> ms::BasicSurface::parent() const
{
    return parent_.lock();
}

namespace
{
//This class avoids locking for long periods of time by copying (or lazy-copying)
class SurfaceSnapshot : public mg::Renderable
{
public:
    SurfaceSnapshot(
        std::shared_ptr<mc::BufferStream::Submission> buffer,
        geom::Point top_left,
        std::optional<geom::Rectangle> const& clip_area,
        glm::mat4 const& transform,
        MirOrientation orientation,
        MirMirrorMode mirror_mode,
        float alpha,
        mg::Renderable::ID id,
        ms::Surface const* surface)
    : entry{std::move(buffer)},
      alpha_{alpha},
      screen_position_{top_left, entry->size()},
      clip_area_{clip_area},
      transformation_{transform},
      orientation_{orientation},
      mirror_mode_{mirror_mode},
      id_{id},
      surface{surface}
    {
    }

    ~SurfaceSnapshot()
    {
    }

    std::shared_ptr<mg::Buffer> buffer() const override
    {
        return entry->claim_buffer();
    }

    geom::Rectangle screen_position() const override
    { return screen_position_; }

    geom::RectangleD src_bounds() const override
    { return entry->source_rect(); }

    std::optional<geom::Rectangle> clip_area() const override
    { return clip_area_; }

    float alpha() const override
    { return alpha_; }

    glm::mat4 transformation() const override
    { return transformation_; }

    MirOrientation orientation() const override
    { return orientation_; }

    MirMirrorMode mirror_mode() const override
    { return mirror_mode_; }

    bool shaped() const override
    {
        return entry->pixel_format().info().transform([](auto info) { return info.has_alpha();}).value_or(true);
    }

    mg::Renderable::ID id() const override
    { return id_; }

    auto surface_if_any() const -> std::optional<mir::scene::Surface const*> override
    {
        return surface;
    }
private:
    std::shared_ptr<mc::BufferStream::Submission> const entry;
    float const alpha_;
    geom::Rectangle const screen_position_;
    std::optional<geom::Rectangle> const clip_area_;
    glm::mat4 const transformation_;
    MirOrientation orientation_;
    MirMirrorMode mirror_mode_;
    mg::Renderable::ID const id_;
    ms::Surface const* surface;
};
}

void ms::BasicSurface::consume(std::shared_ptr<MirEvent const> const& event)
{
    observers->input_consumed(this, event);
}

void ms::BasicSurface::rename(std::string const& title)
{
    auto state = synchronised_state.lock();
    if (state->surface_name != title)
    {
        state->surface_name = title;

        state.drop();

        observers->renamed(this, title.c_str());
    }
}

std::list<ms::StreamInfo> ms::BasicSurface::get_streams() const
{
    return synchronised_state.lock()->layers;
}

void ms::BasicSurface::set_streams(std::list<StreamInfo> const& s)
{
    geom::Point surface_top_left;
    {
        auto state = synchronised_state.lock();
        clear_frame_posted_callbacks(*state);
        state->layers = s;
        update_frame_posted_callbacks(*state);
        surface_top_left = state->surface_rect.top_left;
    }
    observers->moved_to(this, surface_top_left);
}

mg::RenderableList ms::BasicSurface::generate_renderables(mc::CompositorID id) const
{
    auto state = synchronised_state.lock();
    mg::RenderableList list;

    if (state->clip_area)
    {
        if (!state->surface_rect.overlaps(state->clip_area.value()))
            return list;
    }

    auto const content_top_left_ = content_top_left(*state);

    for (auto const& info : state->layers)
    {
        if (info.stream->has_submitted_buffer())
        {
            list.emplace_back(std::make_shared<SurfaceSnapshot>(
                info.stream->next_submission_for_compositor(id),
                content_top_left_ + info.displacement,
                state->clip_area,
                state->transformation_matrix,
                state->orientation,
                state->mirror_mode,
                state->surface_alpha,
                info.stream.get(),
                this));
        }
    }
    return list;
}

void ms::BasicSurface::set_confine_pointer_state(MirPointerConfinementState state)
{
    synchronised_state.lock()->confine_pointer_state = state;
}

MirPointerConfinementState ms::BasicSurface::confine_pointer_state() const
{
    return synchronised_state.lock()->confine_pointer_state;
}

void ms::BasicSurface::placed_relative(geometry::Rectangle const& placement)
{
    observers->placed_relative(this, placement);
}

auto mir::scene::BasicSurface::depth_layer() const -> MirDepthLayer
{
    return synchronised_state.lock()->depth_layer;
}

void mir::scene::BasicSurface::set_depth_layer(MirDepthLayer depth_layer)
{
    synchronised_state.lock()->depth_layer = depth_layer;
    observers->depth_layer_set_to(this, depth_layer);
}

auto mir::scene::BasicSurface::visible_on_lock_screen() const -> bool
{
    return synchronised_state.lock()->visible_on_lock_screen;
}

void mir::scene::BasicSurface::set_visible_on_lock_screen(bool visible)
{
    synchronised_state.lock()->visible_on_lock_screen = visible;
}

std::optional<geom::Rectangle> mir::scene::BasicSurface::clip_area() const
{
    return synchronised_state.lock()->clip_area;
}

void mir::scene::BasicSurface::set_clip_area(std::optional<geom::Rectangle> const& area)
{
    synchronised_state.lock()->clip_area = area;
}

auto mir::scene::BasicSurface::focus_state() const -> MirWindowFocusState
{
    return synchronised_state.lock()->focus;
}

void mir::scene::BasicSurface::set_focus_state(MirWindowFocusState new_state)
{
    if (new_state != mir_window_focus_state_focused &&
        new_state != mir_window_focus_state_active &&
        new_state != mir_window_focus_state_unfocused)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid focus state."));
    }

    auto state = synchronised_state.lock();
    if (state->focus != new_state)
    {
        state->focus = new_state;

        state.drop();
        observers->attrib_changed(this, mir_window_attrib_focus, new_state);
    }
}

auto mir::scene::BasicSurface::application_id() const -> std::string
{
    return synchronised_state.lock()->application_id;
}

void mir::scene::BasicSurface::set_application_id(std::string const& application_id)
{
    auto state = synchronised_state.lock();
    if (state->application_id != application_id)
    {
        state->application_id = application_id;

        state.drop();
        observers->application_id_set_to(this, application_id);
    }
}

auto mir::scene::BasicSurface::session() const -> std::weak_ptr<Session>
{
    return {};
}

void mir::scene::BasicSurface::set_window_margins(
    geom::DeltaY top,
    geom::DeltaX left,
    geom::DeltaY bottom,
    geom::DeltaX right)
{
    top    = std::max(top,    geom::DeltaY{});
    left   = std::max(left,   geom::DeltaX{});
    bottom = std::max(bottom, geom::DeltaY{});
    right  = std::max(right,  geom::DeltaX{});

    auto state = synchronised_state.lock();
    if (top    != state->margins.top    ||
        left   != state->margins.left   ||
        bottom != state->margins.bottom ||
        right  != state->margins.right)
    {
        state->margins.top    = top;
        state->margins.left   = left;
        state->margins.bottom = bottom;
        state->margins.right  = right;

        update_frame_posted_callbacks(*state);

        auto const size = content_size(*state);

        state.drop();

        observers->content_resized_to(this, size);
    }
}

auto mir::scene::BasicSurface::focus_mode() const -> MirFocusMode
{
    return synchronised_state.lock()->focus_mode;
}

void mir::scene::BasicSurface::set_focus_mode(MirFocusMode focus_mode)
{
    synchronised_state.lock()->focus_mode = focus_mode;
}

auto mir::scene::BasicSurface::tiled_edges() const -> Flags<MirTiledEdge>
{
    return synchronised_state.lock()->tiled_edges;
}

void mir::scene::BasicSurface::set_tiled_edges(Flags<MirTiledEdge> edges)
{
    synchronised_state.lock()->tiled_edges = edges;
    observers->tiled_edges(this, edges);
}

void mir::scene::BasicSurface::clear_frame_posted_callbacks(State& state)
{
    for (auto& layer : state.layers)
    {
        layer.stream->set_frame_posted_callback([](auto){});
    }
}

void mir::scene::BasicSurface::update_frame_posted_callbacks(State& state)
{
    for (auto& layer : state.layers)
    {
        auto const surface_local_position = geom::Point{} + state.margins.left + state.margins.top + layer.displacement;
        layer.stream->set_frame_posted_callback(
            [this, observers=std::weak_ptr{observers}, surface_local_position]
                (auto const& size)
            {
                if (auto const o = observers.lock())
                {
                    o->frame_posted(this, geom::Rectangle{surface_local_position, size});
                }
            });
    }
}

auto mir::scene::BasicSurface::content_size(State const& state) const -> geometry::Size
{
    return geom::Size{
        std::max(state.surface_rect.size.width - state.margins.left - state.margins.right, geom::Width{0}),
        std::max(state.surface_rect.size.height - state.margins.top - state.margins.bottom, geom::Height{0})};
}

auto mir::scene::BasicSurface::content_top_left(State const& state) const -> geometry::Point
{
    return state.surface_rect.top_left + geom::Displacement{state.margins.left, state.margins.top};
}

void mir::scene::BasicSurface::track_outputs()
{
    auto const state{synchronised_state.lock()};
    decltype(tracked_output_scales) tracked;

    display_config->for_each_output(
        [&](mg::DisplayConfigurationOutput const& output)
        {
            if (output.valid() && output.used && output.extents().overlaps(state->surface_rect))
            {
                if (!tracked_output_scales.contains(output.id))
                {
                    observers->entered_output(this, output.id);
                }
                tracked.emplace(output.id, output.scale);
            }
        });

    for (auto const& [id, scale] : tracked_output_scales)
    {
        if (auto new_entry = tracked.find(id); new_entry == tracked.end())
        {
            observers->left_output(this, id);
        }
        else if (new_entry->second != scale)
        {
            observers->rescale_output(this, id);
        }
    }

    tracked_output_scales = std::move(tracked);
}

void mir::scene::BasicSurface::linearised_track_outputs()
{
    thread_local bool const on_wayland_thread = []()
        {
            static size_t const max_thread_name_size = 16;
            char thread_name[max_thread_name_size];
            pthread_getname_np(pthread_self(), thread_name, sizeof thread_name);
            return strcmp("Mir/Wayland", thread_name) == 0;
        }();

    if (on_wayland_thread)
    {
        // Since work in the Wayland thread is executed eagerly, the enter/leave
        // notifications may race with the output tracking code in other threads.
        // To prevent this, we wait for the work to be executed in a linearising
        // executor.
        std::latch latch{1};
        linearising_executor.spawn([this, &latch]
            {
                track_outputs();
                latch.count_down();
            });
        latch.wait();
    }
    else
    {
        track_outputs();
    }
}
