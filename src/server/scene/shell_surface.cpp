/*
* Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "mir/scene/shell_surface.h"
#include "mir/observer_multiplexer.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/graphics/cursor_image.h"
#include "mir/scene/surface_observer.h"
#include "mir/renderer/sw/pixel_source.h"

namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace mw = mir::wayland;

class ms::ShellSurface::Multiplexer : public mir::ObserverMultiplexer<ms::SurfaceObserver>
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
};

ms::ShellSurface::ShellSurface(
    std::string const& name,
    geometry::Rectangle area,
    std::shared_ptr<compositor::BufferStream> const& stream)
    : observers(std::make_unique<Multiplexer>()),
      stream(stream)
{
    synchronised_state.lock()->surface_name = name;
    synchronised_state.lock()->surface_rect = area;
    // TODO (mattkae): Notify the scene report of my birth
}

ms::ShellSurface::~ShellSurface()
{
    // TODO (mattkae): Notify the scene report of my death
}

void ms::ShellSurface::register_interest(std::weak_ptr<SurfaceObserver> const& observer)
{
    observers->register_interest(observer);
}

void ms::ShellSurface::register_interest(std::weak_ptr<SurfaceObserver> const& observer, Executor& executor)
{
    observers->register_interest(observer, executor);
}

void ms::ShellSurface::register_early_observer(std::weak_ptr<SurfaceObserver> const& observer, Executor& executor)
{
    observers->register_early_observer(observer, executor);
}

void ms::ShellSurface::unregister_interest(SurfaceObserver const& observer)
{
    observers->unregister_interest(observer);
}

void ms::ShellSurface::initial_placement_done()
{
    // TODO (mattkae): no-op?
}

std::string ms::ShellSurface::name() const
{
    return synchronised_state.lock()->surface_name;
}

void ms::ShellSurface::move_to(geometry::Point const& top_left)
{
    synchronised_state.lock()->surface_rect.top_left = top_left;
    observers->moved_to(this, top_left);
}

void ms::ShellSurface::set_hidden(bool hide)
{
    synchronised_state.lock()->hidden = hide;
    observers->hidden_set_to(this, hide);
}

mir::geometry::Size ms::ShellSurface::window_size() const
{
    return synchronised_state.lock()->surface_rect.size;
}

mir::geometry::Displacement ms::ShellSurface::content_offset() const
{
    return geom::Displacement{0, 0};
}

mir::geometry::Size ms::ShellSurface::content_size() const
{
    return window_size();
}

std::shared_ptr<mf::BufferStream> ms::ShellSurface::primary_buffer_stream() const
{
    return stream;
}

void ms::ShellSurface::set_input_region(std::vector<geom::Rectangle> const&)
{
    // TODO (mattkae): no-op?
}

std::vector<geom::Rectangle> ms::ShellSurface::get_input_region() const
{
    return { synchronised_state.lock()->surface_rect };
}

void ms::ShellSurface::resize(geom::Size const& desired_size)
{
    geom::Size new_size = desired_size;
    if (new_size.width <= geom::Width{0})   new_size.width = geom::Width{1};
    if (new_size.height <= geom::Height{0}) new_size.height = geom::Height{1};

    auto state = synchronised_state.lock();
    if (new_size != state->surface_rect.size)
    {
        state->surface_rect.size = new_size;
        state.drop();

        observers->window_resized_to(this, new_size);
        observers->content_resized_to(this, new_size);
    }
}

geom::Point ms::ShellSurface::top_left() const
{
    return synchronised_state.lock()->surface_rect.top_left;
}

geom::Rectangle ms::ShellSurface::input_bounds() const
{
    auto state = synchronised_state.lock();
    return state->surface_rect;
}

bool ms::ShellSurface::input_area_contains(geom::Point const& point) const
{
    auto state = synchronised_state.lock();

    if (!visible())
        return false;

    auto const local_point = as_point(point - state->surface_rect.top_left);
    if (state->surface_rect.contains(local_point))
        return true;
    return false;
}

void ms::ShellSurface::set_alpha(float alpha)
{
    synchronised_state.lock()->surface_alpha = alpha;
    observers->alpha_set_to(this, alpha);
}

void ms::ShellSurface::set_orientation(MirOrientation orientation)
{
    observers->orientation_set_to(this, orientation);
}

void ms::ShellSurface::set_transformation(glm::mat4 const& t)
{
    synchronised_state.lock()->transformation_matrix = t;
    observers->transformation_set_to(this, t);
}

bool ms::ShellSurface::visible() const
{
    return stream->has_submitted_buffer() && !synchronised_state.lock()->hidden;
}

mi::InputReceptionMode ms::ShellSurface::reception_mode() const
{
    return synchronised_state.lock()->input_mode;
}

void ms::ShellSurface::set_reception_mode(mi::InputReceptionMode mode)
{
    synchronised_state.lock()->input_mode = mode;
    observers->reception_mode_set_to(this, mode);
}

MirWindowType ms::ShellSurface::type() const
{
    return synchronised_state.lock()->type;
}

MirWindowType ms::ShellSurface::set_type(MirWindowType t)
{
    if (t < mir_window_type_normal || t >= mir_window_types)
    {
        throw std::logic_error("Invalid surface type.");
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

MirWindowState ms::ShellSurface::state() const
{
    return synchronised_state.lock()->state.active_state();
}

MirWindowState ms::ShellSurface::set_state(MirWindowState s)
{
    if (s < mir_window_state_unknown || s >= mir_window_states)
        throw std::logic_error("Invalid surface state.");

    auto state = synchronised_state.lock();
    if (state->state.active_state() != s)
    {
        state->state = state->state.with_active_state(s);

        state.drop();

        observers->attrib_changed(this, mir_window_attrib_state, s);
    }

    return s;
}

auto ms::ShellSurface::state_tracker() const -> SurfaceStateTracker
{
    return synchronised_state.lock()->state;
}

MirOrientationMode ms::ShellSurface::set_preferred_orientation(MirOrientationMode new_orientation_mode)
{
    if ((new_orientation_mode & mir_orientation_mode_any) == 0)
    {
        throw std::logic_error("Invalid orientation mode");
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

int ms::ShellSurface::configure(MirWindowAttrib attrib, int value)
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
        throw std::logic_error("Invalid surface attribute.");
    }

    return result;
}

int ms::ShellSurface::query(MirWindowAttrib attrib) const
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
        default: throw std::logic_error("Invalid surface attribute.");
    }
}

void ms::ShellSurface::hide()
{
    set_hidden(true);
}

void ms::ShellSurface::show()
{
    set_hidden(false);
}

void ms::ShellSurface::set_cursor_image(std::shared_ptr<mg::CursorImage> const& image)
{
    synchronised_state.lock()->cursor_image = image;

    if (image)
        observers->cursor_image_set_to(this, image);
    else
        observers->cursor_image_removed(this);
}

void ms::ShellSurface::remove_cursor_image()
{
    synchronised_state.lock()->cursor_image = nullptr;
    observers->cursor_image_removed(this);
}

std::shared_ptr<mg::CursorImage> ms::ShellSurface::cursor_image() const
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

void ms::ShellSurface::set_cursor_from_buffer(
    std::shared_ptr<mg::Buffer> buffer,
    geom::Displacement const& hotspot)
{
    auto image = std::make_shared<CursorImageFromBuffer>(buffer, hotspot);
    synchronised_state.lock()->cursor_image = image;
    observers->cursor_image_set_to(this, image);
}

auto ms::ShellSurface::wayland_surface() -> mw::Weak<mf::WlSurface> const&
{
    return wayland_surface_;
}

void ms::ShellSurface::request_client_surface_close()
{
    observers->client_surface_close_requested(this);
}

int ms::ShellSurface::dpi() const
{
    return synchronised_state.lock()->dpi;
}

int ms::ShellSurface::set_dpi(int new_dpi)
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

MirWindowVisibility ms::ShellSurface::set_visibility(MirWindowVisibility new_visibility)
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

std::shared_ptr<ms::Surface> ms::ShellSurface::parent() const
{
    // TODO (mattkae): support parent surfaces?
    return nullptr;
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
        glm::mat4 const& transform,
        float alpha,
        mg::Renderable::ID id,
        ms::Surface const* surface)
    : entry{std::move(buffer)},
      alpha_{alpha},
      screen_position_{top_left, entry->size()},
      transformation_{transform},
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
    { return std::nullopt; }

    float alpha() const override
    { return alpha_; }

    glm::mat4 transformation() const override
    { return transformation_; }

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
    glm::mat4 const transformation_;
    mg::Renderable::ID const id_;
    ms::Surface const* surface;
};
}

void ms::ShellSurface::consume(std::shared_ptr<MirEvent const> const& event)
{
    observers->input_consumed(this, event);
}

void ms::ShellSurface::rename(std::string const& title)
{
    auto state = synchronised_state.lock();
    if (state->surface_name != title)
    {
        state->surface_name = title;

        state.drop();

        observers->renamed(this, title.c_str());
    }
}

void ms::ShellSurface::set_streams(std::list<scene::StreamInfo> const&)
{
    // TODO (mattkae): unsupported
}

mg::RenderableList ms::ShellSurface::generate_renderables(mc::CompositorID id) const
{
    auto state = synchronised_state.lock();
    mg::RenderableList list;

    auto const content_top_left_ = state->surface_rect.top_left;

    if (stream->has_submitted_buffer())
    {
        list.emplace_back(std::make_shared<SurfaceSnapshot>(
            stream->next_submission_for_compositor(id),
            content_top_left_,
            state->transformation_matrix,
            state->surface_alpha,
            stream.get(),
            this));
    }
    return list;
}

void ms::ShellSurface::set_confine_pointer_state(MirPointerConfinementState state)
{
    synchronised_state.lock()->confine_pointer_state = state;
}

MirPointerConfinementState ms::ShellSurface::confine_pointer_state() const
{
    return synchronised_state.lock()->confine_pointer_state;
}

void ms::ShellSurface::placed_relative(geometry::Rectangle const& placement)
{
    observers->placed_relative(this, placement);
}

auto mir::scene::ShellSurface::depth_layer() const -> MirDepthLayer
{
    return synchronised_state.lock()->depth_layer;
}

void mir::scene::ShellSurface::set_depth_layer(MirDepthLayer depth_layer)
{
    synchronised_state.lock()->depth_layer = depth_layer;
    observers->depth_layer_set_to(this, depth_layer);
}

auto mir::scene::ShellSurface::visible_on_lock_screen() const -> bool
{
    return synchronised_state.lock()->visible_on_lock_screen;
}

void mir::scene::ShellSurface::set_visible_on_lock_screen(bool visible)
{
    synchronised_state.lock()->visible_on_lock_screen = visible;
}

std::optional<geom::Rectangle> mir::scene::ShellSurface::clip_area() const
{
    // TODO (mattkae)
    return std::nullopt;
}

void mir::scene::ShellSurface::set_clip_area(std::optional<geom::Rectangle> const&)
{
    // TODO (mattkae)
}

auto mir::scene::ShellSurface::focus_state() const -> MirWindowFocusState
{
    return synchronised_state.lock()->focus;
}

void mir::scene::ShellSurface::set_focus_state(MirWindowFocusState new_state)
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

auto mir::scene::ShellSurface::application_id() const -> std::string
{
    return synchronised_state.lock()->application_id;
}

void mir::scene::ShellSurface::set_application_id(std::string const& application_id)
{
    auto state = synchronised_state.lock();
    if (state->application_id != application_id)
    {
        state->application_id = application_id;

        state.drop();
        observers->application_id_set_to(this, application_id);
    }
}

auto mir::scene::ShellSurface::session() const -> std::weak_ptr<Session>
{
    return {};
}

void mir::scene::ShellSurface::set_window_margins(
    geom::DeltaY,
    geom::DeltaX,
    geom::DeltaY,
    geom::DeltaX)
{
    // TODO: Support window margins
}

auto mir::scene::ShellSurface::focus_mode() const -> MirFocusMode
{
    return synchronised_state.lock()->focus_mode;
}

void mir::scene::ShellSurface::set_focus_mode(MirFocusMode focus_mode)
{
    synchronised_state.lock()->focus_mode = focus_mode;
}
