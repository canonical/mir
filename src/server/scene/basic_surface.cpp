/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
#include "mir/input/input_channel.h"
#include "mir/shell/input_targeter.h"
#include "mir/graphics/buffer.h"

#include "mir/scene/scene_report.h"
#include "mir/scene/surface_configurator.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <algorithm>

namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;

void ms::SurfaceObservers::attrib_changed(MirSurfaceAttrib attrib, int value)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    // TBD Maybe we should copy observers so we can release the lock?
    for (auto const& p : observers)
        p->attrib_changed(attrib, value);
}

void ms::SurfaceObservers::resized_to(geometry::Size const& size)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    // TBD Maybe we should copy observers so we can release the lock?
    for (auto const& p : observers)
        p->resized_to(size);
}

void ms::SurfaceObservers::moved_to(geometry::Point const& top_left)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    // TBD Maybe we should copy observers so we can release the lock?
    for (auto const& p : observers)
        p->moved_to(top_left);
}

void ms::SurfaceObservers::hidden_set_to(bool hide)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    // TBD Maybe we should copy observers so we can release the lock?
    for (auto const& p : observers)
        p->hidden_set_to(hide);
}

void ms::SurfaceObservers::frame_posted()
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    // TBD Maybe we should copy observers so we can release the lock?
    for (auto const& p : observers)
        p->frame_posted();
}

void ms::SurfaceObservers::alpha_set_to(float alpha)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    // TBD Maybe we should copy observers so we can release the lock?
    for (auto const& p : observers)
        p->alpha_set_to(alpha);
}

void ms::SurfaceObservers::transformation_set_to(glm::mat4 const& t)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    // TBD Maybe we should copy observers so we can release the lock?
    for (auto const& p : observers)
        p->transformation_set_to(t);
}

void ms::SurfaceObservers::add(std::shared_ptr<SurfaceObserver> const& observer)
{
    if (observer)
    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        observers.push_back(observer);
    }
}

void ms::SurfaceObservers::remove(std::shared_ptr<SurfaceObserver> const& observer)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    observers.erase(std::remove(observers.begin(),observers.end(), observer), observers.end());
}

ms::BasicSurface::BasicSurface(
    std::string const& name,
    geometry::Rectangle rect,
    bool nonrectangular,
    std::shared_ptr<compositor::BufferStream> const& buffer_stream,
    std::shared_ptr<input::InputChannel> const& input_channel,
    std::shared_ptr<SurfaceConfigurator> const& configurator,
    std::shared_ptr<SceneReport> const& report) :
    surface_name(name),
    surface_rect(rect),
    surface_alpha(1.0f),
    first_frame_posted(false),
    hidden(false),
    nonrectangular(nonrectangular),
    input_rectangles{surface_rect},
    surface_buffer_stream(buffer_stream),
    server_input_channel(input_channel),
    configurator(configurator),
    report(report),
    type_value(mir_surface_type_normal),
    state_value(mir_surface_state_restored)
{
    report->surface_created(this, surface_name);
}

void ms::BasicSurface::force_requests_to_complete()
{
    surface_buffer_stream->force_requests_to_complete();
}

ms::BasicSurface::~BasicSurface() noexcept
{
    report->surface_deleted(this, surface_name);
}

std::shared_ptr<mc::BufferStream> ms::BasicSurface::buffer_stream() const
{
    return surface_buffer_stream;
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

float ms::BasicSurface::alpha() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_alpha;
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

MirPixelFormat ms::BasicSurface::pixel_format() const
{
    return surface_buffer_stream->get_stream_pixel_format();
}

void ms::BasicSurface::swap_buffers(mg::Buffer* old_buffer, std::function<void(mg::Buffer* new_buffer)> complete)
{
    bool const posting{!!old_buffer};

    surface_buffer_stream->swap_client_buffers(old_buffer, complete);

    if (posting)
    {
        {
            std::unique_lock<std::mutex> lk(guard);
            first_frame_posted = true;
        }
        observers.frame_posted();
    }
}

void ms::BasicSurface::allow_framedropping(bool allow)
{
    surface_buffer_stream->allow_framedropping(allow);
}

std::shared_ptr<mg::Buffer> ms::BasicSurface::snapshot_buffer() const
{
    return surface_buffer_stream->lock_snapshot_buffer();
}

bool ms::BasicSurface::supports_input() const
{
    if (server_input_channel)
        return true;
    return false;
}

int ms::BasicSurface::client_input_fd() const
{
    if (!supports_input())
        BOOST_THROW_EXCEPTION(std::logic_error("Surface does not support input"));
    return server_input_channel->client_fd();
}

std::shared_ptr<mi::InputChannel> ms::BasicSurface::input_channel() const
{
    return server_input_channel;
}

void ms::BasicSurface::set_input_region(std::vector<geom::Rectangle> const& input_rectangles)
{
    std::unique_lock<std::mutex> lock(guard);
    this->input_rectangles = input_rectangles;
}

void ms::BasicSurface::resize(geom::Size const& size)
{
    if (size == this->size())
        return;

    if (size.width <= geom::Width{0} || size.height <= geom::Height{0})
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Impossible resize requested"));
    }
    /*
     * Other combinations may still be invalid (like dimensions too big or
     * insufficient resources), but those are runtime and platform-specific, so
     * not predictable here. Such critical exceptions would arise from
     * the platform buffer allocator as a runtime_error via:
     */
    surface_buffer_stream->resize(size);

    // Now the buffer stream has successfully resized, update the state second;
    {
        std::unique_lock<std::mutex> lock(guard);
        surface_rect.size = size;
    }
    observers.resized_to(size);
}

geom::Point ms::BasicSurface::top_left() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_rect.top_left;
}

bool ms::BasicSurface::contains(geom::Point const& point) const
{
    std::unique_lock<std::mutex> lock(guard);
    if (hidden)
        return false;

    for (auto const& rectangle : input_rectangles)
    {
        if (rectangle.contains(point))
        {
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
    return !hidden && first_frame_posted;
} 

void ms::BasicSurface::with_most_recent_buffer_do(
    std::function<void(mg::Buffer&)> const& exec)
{
    auto buf = snapshot_buffer();
    exec(*buf);
}


MirSurfaceType ms::BasicSurface::type() const
{
    return type_value;
}

bool ms::BasicSurface::set_type(MirSurfaceType t)
{
    bool valid = false;

    if (t >= 0 && t < mir_surface_types)
    {
        type_value = t;
        valid = true;
    }

    return valid;
}

MirSurfaceState ms::BasicSurface::state() const
{
    return state_value;
}

bool ms::BasicSurface::set_state(MirSurfaceState s)
{
    bool valid = false;

    if (s > mir_surface_state_unknown &&
        s < mir_surface_states)
    {
        state_value = s;
        valid = true;

        observers.attrib_changed(mir_surface_attrib_state, s);
    }

    return valid;
}

void ms::BasicSurface::take_input_focus(std::shared_ptr<msh::InputTargeter> const& targeter)
{
    targeter->focus_changed(input_channel());
}

int ms::BasicSurface::configure(MirSurfaceAttrib attrib, int value)
{
    int result = 0;
    bool allow_dropping = false;
    /*
     * TODO: In future, query the shell implementation for the subset of
     *       attributes/types it implements.
     */
    value = configurator->select_attribute_value(*this, attrib, value);
    switch (attrib)
    {
    case mir_surface_attrib_type:
        if (!set_type(static_cast<MirSurfaceType>(value)))
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface "
                                                   "type."));
        result = type();
        break;
    case mir_surface_attrib_state:
        if (value != mir_surface_state_unknown &&
            !set_state(static_cast<MirSurfaceState>(value)))
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface state."));
        result = state();
        break;
    case mir_surface_attrib_focus:
        observers.attrib_changed(attrib, value);
        break;
    case mir_surface_attrib_swapinterval:
        allow_dropping = (value == 0);
        allow_framedropping(allow_dropping);
        result = value;
        break;
    default:
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface "
                                               "attribute."));
        break;
    }

    configurator->attribute_set(*this, attrib, result);

    return result;
}

void ms::BasicSurface::hide()
{
    set_hidden(true);
}

void ms::BasicSurface::show()
{
    set_hidden(false);
}


void ms::BasicSurface::add_observer(std::shared_ptr<SurfaceObserver> const& observer)
{
    observers.add(observer);
}

void ms::BasicSurface::remove_observer(std::weak_ptr<SurfaceObserver> const& observer)
{
    auto o = observer.lock();
    if (!o)
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid observer (previously destroyed)"));
    observers.remove(o);
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
        bool visible,
        bool alpha_enabled,
        float alpha,
        bool shaped,
        mg::Renderable::ID id)
    : underlying_buffer_stream{stream},
      compositor_buffer{nullptr},
      compositor_id{compositor_id},
      alpha_enabled_{alpha_enabled},
      alpha_{alpha},
      shaped_{shaped},
      visible_{visible},
      screen_position_(position),
      transformation_(transform),
      id_(id)
    {
    }

    ~SurfaceSnapshot()
    {
    }
 
    int buffers_ready_for_compositor() const override
    { return underlying_buffer_stream->buffers_ready_for_compositor(); }

    std::shared_ptr<mg::Buffer> buffer() const override
    {
        if (!compositor_buffer)
            compositor_buffer = underlying_buffer_stream->lock_compositor_buffer(compositor_id);
        return compositor_buffer;
    }

    bool visible() const override
    { return visible_; }

    bool alpha_enabled() const override
    { return alpha_enabled_; }

    geom::Rectangle screen_position() const override
    { return screen_position_; }

    float alpha() const override
    { return alpha_; }

    glm::mat4 transformation() const override
    { return transformation_; }

    bool shaped() const override
    { return shaped_; }
 
    mg::Renderable::ID id() const override
    { return id_; }

private:
    std::shared_ptr<mc::BufferStream> const underlying_buffer_stream;
    std::shared_ptr<mg::Buffer> mutable compositor_buffer;
    void const*const compositor_id;
    bool const alpha_enabled_;
    float const alpha_;
    bool const shaped_;
    bool const visible_;
    geom::Rectangle const screen_position_;
    glm::mat4 const transformation_;
    mg::Renderable::ID const id_; 
};
}

std::unique_ptr<mg::Renderable> ms::BasicSurface::compositor_snapshot(void const* compositor_id) const
{
    std::unique_lock<std::mutex> lk(guard);

    auto const shaped = nonrectangular || (surface_alpha < 1.0f);
    return std::unique_ptr<mg::Renderable>(
        new SurfaceSnapshot(
            surface_buffer_stream,
            compositor_id,
            surface_rect,
            transformation_matrix,
            visible(lk),
            shaped,
            surface_alpha,
            nonrectangular, 
            this));
}
