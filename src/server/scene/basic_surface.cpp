/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 *
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "basic_surface.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/frontend/event_sink.h"
#include "mir/shell/input_targeter.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/cursor_image.h"
#include "mir/graphics/pixel_format_utils.h"
#include "mir/geometry/displacement.h"
#include "mir/renderer/sw/pixel_source.h"

#include "mir/scene/scene_report.h"
#include "mir/scene/null_surface_observer.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <algorithm>

#include <string.h> // memcpy

namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mf = mir::frontend;
namespace geom = mir::geometry;
namespace mrs = mir::renderer::software;

void ms::SurfaceObservers::attrib_changed(MirWindowAttrib attrib, int value)
{
    for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
        { observer->attrib_changed(attrib, value); });
}

void ms::SurfaceObservers::resized_to(geometry::Size const& size)
{
    for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
        { observer->resized_to(size); });
}

void ms::SurfaceObservers::moved_to(geometry::Point const& top_left)
{
    for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
        { observer->moved_to(top_left); });
}

void ms::SurfaceObservers::hidden_set_to(bool hide)
{
    for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
        { observer->hidden_set_to(hide); });
}

void ms::SurfaceObservers::frame_posted(int frames_available, geometry::Size const& size)
{
    for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
        { observer->frame_posted(frames_available, size); });
}

void ms::SurfaceObservers::alpha_set_to(float alpha)
{
    for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
        { observer->alpha_set_to(alpha); });
}

void ms::SurfaceObservers::orientation_set_to(MirOrientation orientation)
{
    for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
        { observer->orientation_set_to(orientation); });
}

void ms::SurfaceObservers::transformation_set_to(glm::mat4 const& t)
{
    for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
        { observer->transformation_set_to(t); });
}

void ms::SurfaceObservers::cursor_image_set_to(mg::CursorImage const& image)
{
    for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
        { observer->cursor_image_set_to(image); });
}

void ms::SurfaceObservers::reception_mode_set_to(mi::InputReceptionMode mode)
{
    for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
        { observer->reception_mode_set_to(mode); });
}

void ms::SurfaceObservers::client_surface_close_requested()
{
    for_each([](std::shared_ptr<SurfaceObserver> const& observer)
        { observer->client_surface_close_requested(); });
}

void ms::SurfaceObservers::keymap_changed(MirInputDeviceId id, std::string const& model, std::string const& layout,
                                          std::string const& variant, std::string const& options)
{
    for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
        { observer->keymap_changed(id, model, layout, variant, options); });
}

void ms::SurfaceObservers::renamed(char const* name)
{
    for_each([name](std::shared_ptr<SurfaceObserver> const& observer)
        { observer->renamed(name); });
}

void ms::SurfaceObservers::cursor_image_removed()
{
    for_each([](std::shared_ptr<SurfaceObserver> const& observer)
        { observer->cursor_image_removed(); });
}

void ms::SurfaceObservers::placed_relative(geometry::Rectangle const& placement)
{
    for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
                 { observer->placed_relative(placement); });
}

void ms::SurfaceObservers::input_consumed(MirEvent const* event)
{
    for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
                 { observer->input_consumed(event); });
}

void ms::SurfaceObservers::start_drag_and_drop(std::vector<uint8_t> const& handle)
{
    for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
                 { observer->start_drag_and_drop(handle); });
}


struct ms::CursorStreamImageAdapter
{
    CursorStreamImageAdapter(ms::BasicSurface &surface)
        : surface(surface),
          observer{std::make_shared<FramePostObserver>(this)}
    {
    }

    ~CursorStreamImageAdapter()
    {
        reset();
    }

    void reset()
    {
        if (stream)
        {
            stream->remove_observer(observer);
            stream.reset();
        }
    }

    void update(std::shared_ptr<mf::BufferStream> const& new_stream, geom::Displacement const& new_hotspot)
    {
        if (new_stream == stream && new_hotspot == hotspot)
        {
            return;
        }
        else if (new_stream != stream)
        {
            if (stream) stream->remove_observer(observer);

            stream = std::dynamic_pointer_cast<mc::BufferStream>(new_stream);
            stream->add_observer(observer);
        }

        hotspot = new_hotspot;
        post_cursor_image_from_current_buffer();
    }

private:
    struct FramePostObserver : public ms::NullSurfaceObserver
    {
        FramePostObserver(CursorStreamImageAdapter const* self)
            : self(self)
        {
        }

        void frame_posted(int /* available */, geometry::Size const& /* size */)
        {
            self->post_cursor_image_from_current_buffer();
        }

        CursorStreamImageAdapter const* const self;
    };

    void post_cursor_image_from_current_buffer() const
    {
        surface.set_cursor_from_buffer(*stream->lock_compositor_buffer(this), hotspot);
    }

    ms::BasicSurface& surface;
    std::shared_ptr<FramePostObserver> const observer;

    std::shared_ptr<mc::BufferStream> stream;
    geom::Displacement hotspot;
};

namespace
{
//TODO: the concept of default stream is going away very soon.
std::shared_ptr<mc::BufferStream> default_stream(std::list<ms::StreamInfo> const& layers)
{
    //There's not a good reason, other than soon-to-be-deprecated api to disallow contentless surfaces
    if (layers.empty())
        BOOST_THROW_EXCEPTION(std::logic_error("Surface must have content"));
    else
        return layers.front().stream;
}

}

ms::BasicSurface::BasicSurface(
    std::string const& name,
    geometry::Rectangle rect,
    std::weak_ptr<Surface> const& parent,
    MirPointerConfinementState state,
    std::list<StreamInfo> const& layers,
    std::shared_ptr<mg::CursorImage> const& cursor_image,
    std::shared_ptr<SceneReport> const& report) :
    surface_name(name),
    surface_rect(rect),
    surface_alpha(1.0f),
    hidden(false),
    input_mode(mi::InputReceptionMode::normal),
    custom_input_rectangles(),
    surface_buffer_stream(default_stream(layers)),
    cursor_image_(cursor_image),
    report(report),
    parent_(parent),
    layers(layers),
    confine_pointer_state_(state),
    cursor_stream_adapter{std::make_unique<ms::CursorStreamImageAdapter>(*this)}
{
    report->surface_created(this, surface_name);
}

ms::BasicSurface::BasicSurface(
    std::string const& name,
    geometry::Rectangle rect,
    MirPointerConfinementState state,
    std::list<StreamInfo> const& layers,
    std::shared_ptr<mg::CursorImage> const& cursor_image,
    std::shared_ptr<SceneReport> const& report) :
    BasicSurface(name, rect, std::shared_ptr<Surface>{nullptr}, state, layers,
                 cursor_image, report)
{
}

ms::BasicSurface::~BasicSurface() noexcept
{
    report->surface_deleted(this, surface_name);
}

std::string ms::BasicSurface::name() const
{
    return surface_name;
}

void ms::BasicSurface::move_to(geometry::Point const& top_left)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        surface_rect.top_left = top_left;
    }
    observers.moved_to(top_left);
}

void ms::BasicSurface::set_hidden(bool hide)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        hidden = hide;
    }
    observers.hidden_set_to(hide);
}

mir::geometry::Size ms::BasicSurface::size() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_rect.size;
}

mir::geometry::Size ms::BasicSurface::client_size() const
{
    // TODO: In future when decorated, client_size() would be smaller than size
    return size();
}

std::shared_ptr<mf::BufferStream> ms::BasicSurface::primary_buffer_stream() const
{
    return surface_buffer_stream;
}

void ms::BasicSurface::set_input_region(std::vector<geom::Rectangle> const& input_rectangles)
{
    std::unique_lock<std::mutex> lock(guard);
    custom_input_rectangles = input_rectangles;
}

void ms::BasicSurface::resize(geom::Size const& desired_size)
{
    geom::Size new_size = desired_size;
    if (new_size.width <= geom::Width{0})   new_size.width = geom::Width{1};
    if (new_size.height <= geom::Height{0}) new_size.height = geom::Height{1};

    if (new_size == size())
        return;

    /*
     * Other combinations may still be invalid (like dimensions too big or
     * insufficient resources), but those are runtime and platform-specific, so
     * not predictable here. Such critical exceptions would arise from
     * the platform buffer allocator as a runtime_error via:
     */
    surface_buffer_stream->resize(new_size);

    // Now the buffer stream has successfully resized, update the state second;
    {
        std::unique_lock<std::mutex> lock(guard);
        surface_rect.size = new_size;
    }
    observers.resized_to(new_size);
}

geom::Point ms::BasicSurface::top_left() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_rect.top_left;
}

geom::Rectangle ms::BasicSurface::input_bounds() const
{
    std::unique_lock<std::mutex> lk(guard);

    return surface_rect;
}

// TODO: Does not account for transformation().
bool ms::BasicSurface::input_area_contains(geom::Point const& point) const
{
    std::unique_lock<std::mutex> lock(guard);

    if (!visible(lock))
        return false;

    if (custom_input_rectangles.empty())
    {
        // no custom input, restrict to bounding rectangle
        return surface_rect.contains(point);
    }
    else
    {
        auto local_point = geom::Point{0, 0} + (point-surface_rect.top_left);
        for (auto const& rectangle : custom_input_rectangles)
        {
            if (rectangle.contains(local_point))
                return true;
        }
    }
    return false;
}

void ms::BasicSurface::set_alpha(float alpha)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        surface_alpha = alpha;
    }
    observers.alpha_set_to(alpha);
}

void ms::BasicSurface::set_orientation(MirOrientation orientation)
{
    observers.orientation_set_to(orientation);
}

void ms::BasicSurface::set_transformation(glm::mat4 const& t)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        transformation_matrix = t;
    }
    observers.transformation_set_to(t);
}

bool ms::BasicSurface::visible() const
{
    std::unique_lock<std::mutex> lk(guard);
    return visible(lk); 
}

bool ms::BasicSurface::visible(std::unique_lock<std::mutex>&) const
{
    bool visible{false};
    for (auto const& info : layers)
        visible |= info.stream->has_submitted_buffer();
    return !hidden && visible;
}

mi::InputReceptionMode ms::BasicSurface::reception_mode() const
{
    return input_mode;
}

void ms::BasicSurface::set_reception_mode(mi::InputReceptionMode mode)
{
    {
        std::lock_guard<std::mutex> lk(guard);
        input_mode = mode;
    }
    observers.reception_mode_set_to(mode);
}

MirWindowType ms::BasicSurface::type() const
{    
    std::unique_lock<std::mutex> lg(guard);
    return type_;
}

MirWindowType ms::BasicSurface::set_type(MirWindowType t)
{
    std::unique_lock<std::mutex> lg(guard);
    
    if (t > mir_window_types)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface "
            "type."));
    }

    if (type_ != t)
    {
        type_ = t;
        lg.unlock();

        observers.attrib_changed(mir_window_attrib_type, type_); 
    }

    return t;
}

MirWindowState ms::BasicSurface::state() const
{
    std::unique_lock<std::mutex> lg(guard);
    return state_;
}

MirWindowState ms::BasicSurface::set_state(MirWindowState s)
{
    if (s < mir_window_state_unknown || s > mir_window_states)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface state."));

    std::unique_lock<std::mutex> lg(guard);
    if (state_ != s)
    {
        state_ = s;
        lg.unlock();
        observers.attrib_changed(mir_window_attrib_state, s);
    }

    return s;
}

int ms::BasicSurface::set_swap_interval(int interval)
{
    if (interval < 0)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid swapinterval"));
    }

    std::unique_lock<std::mutex> lg(guard);
    if (swapinterval_ != interval)
    {
        swapinterval_ = interval;
        bool allow_dropping = (interval == 0);
        for (auto& info : layers) 
            info.stream->allow_framedropping(allow_dropping);

        lg.unlock();
        observers.attrib_changed(mir_window_attrib_swapinterval, interval);
    }

    return interval;
}

MirWindowFocusState ms::BasicSurface::set_focus_state(MirWindowFocusState new_state)
{
    if (new_state != mir_window_focus_state_focused &&
        new_state != mir_window_focus_state_unfocused)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid focus state."));
    }

    std::unique_lock<std::mutex> lg(guard);
    if (focus_ != new_state)
    {
        focus_ = new_state;

        lg.unlock();
        observers.attrib_changed(mir_window_attrib_focus, new_state);
    }

    return new_state;
}

MirOrientationMode ms::BasicSurface::set_preferred_orientation(MirOrientationMode new_orientation_mode)
{
    if ((new_orientation_mode & mir_orientation_mode_any) == 0)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid orientation mode"));
    }

    std::unique_lock<std::mutex> lg(guard);
    if (pref_orientation_mode != new_orientation_mode)
    {
        pref_orientation_mode = new_orientation_mode;
        lg.unlock();

        observers.attrib_changed(mir_window_attrib_preferred_orientation, new_orientation_mode);
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
        result = set_focus_state(static_cast<MirWindowFocusState>(result));
        break;
    case mir_window_attrib_swapinterval:
        result = set_swap_interval(result);
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
    std::unique_lock<std::mutex> lg(guard);
    switch (attrib)
    {
        case mir_window_attrib_type: return type_;
        case mir_window_attrib_state: return state_;
        case mir_window_attrib_swapinterval: return swapinterval_;
        case mir_window_attrib_focus: return focus_;
        case mir_window_attrib_dpi: return dpi_;
        case mir_window_attrib_visibility: return visibility_;
        case mir_window_attrib_preferred_orientation: return pref_orientation_mode;
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
    {
        std::unique_lock<std::mutex> lock(guard);
        cursor_stream_adapter->reset();
        
        cursor_image_ = image;
    }

    if (image)
        observers.cursor_image_set_to(*image);
    else
        observers.cursor_image_removed();
}

std::shared_ptr<mg::CursorImage> ms::BasicSurface::cursor_image() const
{
    std::unique_lock<std::mutex> lock(guard);
    return cursor_image_;
}

namespace
{
struct CursorImageFromBuffer : public mg::CursorImage
{
    CursorImageFromBuffer(mg::Buffer &buffer, geom::Displacement const& hotspot)
        : buffer_size(buffer.size()),
          hotspot_(hotspot)
    {
        auto pixel_source = dynamic_cast<mrs::PixelSource*>(buffer.native_buffer_base());
        if (pixel_source)
        {
            pixel_source->read([&](unsigned char const* buffer_pixels)
            {
                size_t buffer_size_bytes = buffer_size.width.as_int() * buffer_size.height.as_int()
                    * MIR_BYTES_PER_PIXEL(buffer.pixel_format());
                pixels = std::unique_ptr<unsigned char[]>(
                    new unsigned char[buffer_size_bytes]
                );
                memcpy(pixels.get(), buffer_pixels, buffer_size_bytes);
            });
        }
        else
        {
            BOOST_THROW_EXCEPTION(std::logic_error("could not read from buffer"));
        }
    }
    void const* as_argb_8888() const
    {
        return pixels.get();
    }

    geom::Size size() const
    {
        return buffer_size;
    }

    geom::Displacement hotspot() const
    {
        return hotspot_;
    }

    geom::Size const buffer_size;
    geom::Displacement const hotspot_;

    std::unique_ptr<unsigned char[]> pixels;
};
}

void ms::BasicSurface::set_cursor_from_buffer(mg::Buffer& buffer, geom::Displacement const& hotspot)
{
    auto image = std::make_shared<CursorImageFromBuffer>(buffer, hotspot);
    {
        std::unique_lock<std::mutex> lock(guard);
        cursor_image_ = image;
    }
    observers.cursor_image_set_to(*image);
}

// In order to set the cursor image from a buffer stream, we use an adapter pattern,
// which observes buffers from the stream and copies them 1 by 1 to cursor images.
// We must be careful, when setting a new cursor image with ms::BasicSurface::set_cursor_image
// we need to reset the stream adapter (to halt the observation and allow the new static image
// to be set). Likewise from the adapter we must use set_cursor_from_buffer as
// opposed to the public set_cursor_from_image in order to avoid resetting the stream
// adapter.
void ms::BasicSurface::set_cursor_stream(std::shared_ptr<mf::BufferStream> const& stream,
                                         geom::Displacement const& hotspot)
{
    cursor_stream_adapter->update(stream, hotspot);
}

void ms::BasicSurface::request_client_surface_close()
{
    observers.client_surface_close_requested();
}

int ms::BasicSurface::dpi() const
{
    std::unique_lock<std::mutex> lock(guard);
    return dpi_;
}

int ms::BasicSurface::set_dpi(int new_dpi)
{
    if (new_dpi < 0)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid DPI value"));
    }

    std::unique_lock<std::mutex> lg(guard);
    if (dpi_ != new_dpi)
    {
        dpi_ = new_dpi;
        lg.unlock();
        observers.attrib_changed(mir_window_attrib_dpi, new_dpi);
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

    std::unique_lock<std::mutex> lg(guard);
    if (visibility_ != new_visibility)
    {
        visibility_ = new_visibility;
        lg.unlock();
        if (new_visibility == mir_window_visibility_exposed)
        {
            for (auto& info : layers)
                info.stream->drop_old_buffers();
        }
        observers.attrib_changed(mir_window_attrib_visibility, visibility_);
    }

    return new_visibility;
}

void ms::BasicSurface::add_observer(std::shared_ptr<SurfaceObserver> const& observer)
{
    observers.add(observer);
    for (auto& info : layers) 
        info.stream->add_observer(observer);
}

void ms::BasicSurface::remove_observer(std::weak_ptr<SurfaceObserver> const& observer)
{
    auto o = observer.lock();
    if (!o)
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid observer (previously destroyed)"));
    observers.remove(o);
    for (auto& info : layers) 
        info.stream->remove_observer(observer);
}

std::shared_ptr<ms::Surface> ms::BasicSurface::parent() const
{
    std::lock_guard<std::mutex> lg(guard);
    return parent_.lock();
}

namespace
{
//This class avoids locking for long periods of time by copying (or lazy-copying)
class SurfaceSnapshot : public mg::Renderable
{
public:
    SurfaceSnapshot(
        std::shared_ptr<mc::BufferStream> const& stream,
        void const* compositor_id,
        geom::Rectangle const& position,
        glm::mat4 const& transform,
        float alpha,
        mg::Renderable::ID id)
    : underlying_buffer_stream{stream},
      compositor_id{compositor_id},
      alpha_{alpha},
      screen_position_(position),
      transformation_(transform),
      id_(id)
    {
    }

    ~SurfaceSnapshot()
    {
    }

    unsigned int swap_interval() const override
    {
        return underlying_buffer_stream->framedropping() ? 0 : 1;
    }
 
    std::shared_ptr<mg::Buffer> buffer() const override
    {
        if (!compositor_buffer)
            compositor_buffer = underlying_buffer_stream->lock_compositor_buffer(compositor_id);
        return compositor_buffer;
    }

    geom::Rectangle screen_position() const override
    { return screen_position_; }

    float alpha() const override
    { return alpha_; }

    glm::mat4 transformation() const override
    { return transformation_; }

    bool shaped() const override
    { return mg::contains_alpha(underlying_buffer_stream->pixel_format()); }
 
    mg::Renderable::ID id() const override
    { return id_; }
private:
    std::shared_ptr<mc::BufferStream> const underlying_buffer_stream;
    std::shared_ptr<mg::Buffer> mutable compositor_buffer;
    void const*const compositor_id;
    float const alpha_;
    geom::Rectangle const screen_position_;
    glm::mat4 const transformation_;
    mg::Renderable::ID const id_;
};
}

int ms::BasicSurface::buffers_ready_for_compositor(void const* id) const
{
    std::unique_lock<std::mutex> lk(guard);
    auto max_buf = 0;
    for (auto const& info : layers)
        max_buf = std::max(max_buf, info.stream->buffers_ready_for_compositor(id));
    return max_buf;
}

void ms::BasicSurface::consume(MirEvent const* event)
{
    observers.input_consumed(event);
}

void ms::BasicSurface::set_keymap(MirInputDeviceId id, std::string const& model, std::string const& layout,
                                  std::string const& variant, std::string const& options)
{
    observers.keymap_changed(id, model, layout, variant, options);
}

void ms::BasicSurface::rename(std::string const& title)
{
    if (surface_name != title)
    {
        surface_name = title;
        observers.renamed(surface_name.c_str());
    }
}

void ms::BasicSurface::set_streams(std::list<scene::StreamInfo> const& s)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        for(auto& layer : layers)
            observers.for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
            {
                layer.stream->remove_observer(observer);
            });

        layers = s;

        for(auto& layer : layers)
            observers.for_each([&](std::shared_ptr<SurfaceObserver> const& observer)
            {
                layer.stream->add_observer(observer);
            });
    }
    observers.moved_to(surface_rect.top_left);
}

mg::RenderableList ms::BasicSurface::generate_renderables(mc::CompositorID id) const
{
    std::unique_lock<std::mutex> lk(guard);
    mg::RenderableList list;
    for (auto const& info : layers)
    {
        if (info.stream->has_submitted_buffer())
        {
            geom::Size size;
            if (info.size.is_set())
                size = info.size.value();
            else
                size = info.stream->stream_size();

            list.emplace_back(std::make_shared<SurfaceSnapshot>(
                info.stream, id,
                geom::Rectangle{surface_rect.top_left + info.displacement, std::move(size)},
                transformation_matrix, surface_alpha, info.stream.get()));
        }
    }
    return list;
}

void ms::BasicSurface::set_confine_pointer_state(MirPointerConfinementState state)
{
    confine_pointer_state_ = state;
}

MirPointerConfinementState ms::BasicSurface::confine_pointer_state() const
{
    return confine_pointer_state_;
}

void ms::BasicSurface::placed_relative(geometry::Rectangle const& placement)
{
    observers.placed_relative(placement);
}

void mir::scene::BasicSurface::start_drag_and_drop(std::vector<uint8_t> const& handle)
{
    observers.start_drag_and_drop(handle);
}
