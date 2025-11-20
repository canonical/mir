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

#include "application_session.h"

#include "mir/scene/surface.h"
#include "mir/scene/session_container.h"
#include "mir/scene/session_listener.h"
#include "mir/scene/surface_factory.h"
#include "mir/shell/surface_stack.h"
#include "mir/shell/surface_specification.h"
#include "mir/compositor/stream.h"
#include "mir/events/event_builders.h"
#include "mir/frontend/event_sink.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/scene/surface_observer.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <memory>
#include <algorithm>
#include <cstring>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mev = mir::events;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

ms::ApplicationSession::ApplicationSession(
    std::shared_ptr<msh::SurfaceStack> const& surface_stack,
    std::shared_ptr<SurfaceFactory> const& surface_factory,
    pid_t pid,
    Fd socket_fd,
    std::string const& session_name,
    std::shared_ptr<SessionListener> const& session_listener,
    std::shared_ptr<mf::EventSink> const& sink,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& gralloc) :
    surface_stack(surface_stack),
    surface_factory(surface_factory),
    pid(pid),
    socket_fd_(socket_fd),
    session_name(session_name),
    session_listener(session_listener),
    event_sink(sink),
    gralloc(gralloc)
{
    assert(surface_stack);
}

ms::ApplicationSession::~ApplicationSession()
{
    std::unique_lock lock(surfaces_and_streams_mutex);
    for (auto const& surface : surfaces)
    {
        session_listener->destroying_surface(*this, surface);
        surface_stack->remove_surface(surface);
    }
}

auto ms::ApplicationSession::create_surface(
    std::shared_ptr<Session> const& session,
    wayland::Weak<frontend::WlSurface> const& wayland_surface,
    shell::SurfaceSpecification const& the_params,
    std::shared_ptr<ms::SurfaceObserver> const& observer,
    Executor* observer_executor) -> std::shared_ptr<Surface>
{
    if (session && session.get() != this)
        fatal_error("Incorrect session");

    //TODO: we take the first stream's content for now. Once the surface factory interface takes more than one stream,
    //      we can take all the streams as content.
    if (!the_params.streams.is_set() || the_params.streams.value().empty())
    {
        BOOST_THROW_EXCEPTION(std::logic_error("surface must have content"));
    }

    auto params = the_params;

    std::shared_ptr<mc::BufferStream> const buffer_stream =
        std::dynamic_pointer_cast<mc::BufferStream>(params.streams.value()[0].stream.lock());

    std::list<StreamInfo> stream_list;
    for (auto& stream : params.streams.value())
    {
        stream_list.push_back({std::dynamic_pointer_cast<mc::BufferStream>(stream.stream.lock()), stream.displacement});
    }

    auto surface = surface_factory->create_surface(session, wayland_surface, stream_list, params);

    auto const input_mode = params.input_mode.is_set() ? params.input_mode.value() : input::InputReceptionMode::normal;
    surface_stack->add_surface(surface, input_mode);

    if (observer && observer_executor)
    {
        surface->register_interest(observer, *observer_executor);
    }
    else if (observer)
    {
        surface->register_interest(observer);
    }

    {
        std::unique_lock lock(surfaces_and_streams_mutex);
        surfaces.push_back(surface);
        default_content_map[surface] = buffer_stream;
    }

    if (observer)
        observer->moved_to(surface.get(), surface->top_left());

    session_listener->surface_created(*this, surface);

    if (params.state.is_set())
        surface->configure(mir_window_attrib_state, params.state.value());
    if (params.type.is_set())
        surface->configure(mir_window_attrib_type, params.type.value());
    if (params.preferred_orientation.is_set())
        surface->configure(mir_window_attrib_preferred_orientation, params.preferred_orientation.value());
    if (params.input_shape.is_set())
        surface->set_input_region(params.input_shape.value());
    if (params.depth_layer.is_set())
        surface->set_depth_layer(params.depth_layer.value());
    if (params.application_id.is_set())
        surface->set_application_id(params.application_id.value());
    if (params.focus_mode.is_set())
        surface->set_focus_mode(params.focus_mode.value());
    if (params.visible_on_lock_screen.is_set())
        surface->set_visible_on_lock_screen(params.visible_on_lock_screen.value());
    if (params.tiled_edges.is_set())
        surface->set_tiled_edges(params.tiled_edges.value());

    return surface;
}

void ms::ApplicationSession::destroy_surface(std::shared_ptr<Surface> const& surface)
{
    {
        std::unique_lock lock(surfaces_and_streams_mutex);

        auto default_content_map_iter = default_content_map.find(surface);
        auto surface_iter = std::find(surfaces.begin(), surfaces.end(), surface);

        if (default_content_map_iter == default_content_map.end() || surface_iter == surfaces.end())
            BOOST_THROW_EXCEPTION(std::runtime_error("Invalid surface"));

        session_listener->destroying_surface(*this, surface);
        default_content_map.erase(default_content_map_iter);
        surfaces.erase(surface_iter);
    }

    surface_stack->remove_surface(surface);
}

std::shared_ptr<ms::Surface> ms::ApplicationSession::surface_after(std::shared_ptr<ms::Surface> const& before) const
{
    std::lock_guard lock(surfaces_and_streams_mutex);
    auto current = surfaces.begin();
    for (; current != surfaces.end(); ++current)
    {
        if (*current == before)
            break;
    }

    if (current == surfaces.end())
        BOOST_THROW_EXCEPTION(std::runtime_error("surface_after: surface is not a member of this session"));

    auto const can_take_focus = [](std::shared_ptr<scene::Surface> const &s)
        {
            switch (s->type())
            {
            case mir_window_type_normal:       /**< AKA "regular"                       */
            case mir_window_type_utility:      /**< AKA "floating"                      */
            case mir_window_type_dialog:
            case mir_window_type_satellite:    /**< AKA "toolbox"/"toolbar"             */
            case mir_window_type_freestyle:
            case mir_window_type_menu:
            case mir_window_type_inputmethod:  /**< AKA "OSK" or handwriting etc.       */
                return true;

            case mir_window_type_gloss:
            case mir_window_type_tip:          /**< AKA "tooltip"                       */
            case mir_window_type_decoration:
            default:
                // Cannot have input focus - skip it
                return false;
            }
        };

    auto next = std::find_if(++current, end(surfaces), can_take_focus);

    if (next == surfaces.end())
        next = std::find_if(begin(surfaces), current, can_take_focus);

    if (next == end(surfaces))
        return {};

    return *next;
}

std::shared_ptr<ms::Surface> ms::ApplicationSession::default_surface() const
{
    std::unique_lock lock(surfaces_and_streams_mutex);

    if (!surfaces.empty())
        return *surfaces.begin();
    else
        return std::shared_ptr<ms::Surface>();
}

std::string ms::ApplicationSession::name() const
{
    return session_name;
}

pid_t ms::ApplicationSession::process_id() const
{
    return pid;
}

mir::Fd ms::ApplicationSession::socket_fd() const
{
    return socket_fd_;
}

void ms::ApplicationSession::hide()
{
    std::unique_lock lock(surfaces_and_streams_mutex);
    for (auto& surface : surfaces)
    {
        surface->hide();
    }
}

void ms::ApplicationSession::show()
{
    std::unique_lock lock(surfaces_and_streams_mutex);
    for (auto& surface : surfaces)
    {
        surface->show();
    }
}

void ms::ApplicationSession::start_prompt_session()
{
    // All sessions which are part of the prompt session get this event.
    event_sink->handle_event(mev::make_prompt_session_state_event(mir_prompt_session_state_started));
}

void ms::ApplicationSession::stop_prompt_session()
{
    event_sink->handle_event(mev::make_prompt_session_state_event(mir_prompt_session_state_stopped));
}

void ms::ApplicationSession::suspend_prompt_session()
{
    event_sink->handle_event(mev::make_prompt_session_state_event(mir_prompt_session_state_suspended));
}

void ms::ApplicationSession::resume_prompt_session()
{
    start_prompt_session();
}

auto ms::ApplicationSession::create_buffer_stream(mg::BufferProperties const& /*props*/)
    -> std::shared_ptr<compositor::BufferStream>
{
    auto stream = std::make_shared<mc::Stream>();
    session_listener->buffer_stream_created(*this, stream);

    std::unique_lock lock(surfaces_and_streams_mutex);
    streams.insert(stream);
    return stream;
}

void ms::ApplicationSession::destroy_buffer_stream(std::shared_ptr<frontend::BufferStream> const& stream)
{
    std::unique_lock lock(surfaces_and_streams_mutex);
    auto stream_it = streams.find(std::dynamic_pointer_cast<compositor::BufferStream>(stream));
    if (stream_it == streams.end())
        BOOST_THROW_EXCEPTION(std::runtime_error("cannot destroy stream: Invalid BufferStream"));

    session_listener->buffer_stream_destroyed(*this, *stream_it);
    streams.erase(stream_it);
}

void ms::ApplicationSession::configure_streams(
    ms::Surface& surface, std::vector<shell::StreamSpecification> const& stream_specs)
{
    std::list<ms::StreamInfo> list;
    for (auto& stream : stream_specs)
    {
        if (auto const s = std::dynamic_pointer_cast<mc::BufferStream>(stream.stream.lock()))
            list.emplace_back(ms::StreamInfo{s, stream.displacement});
    }
    surface.set_streams(list);
}

auto ms::ApplicationSession::has_buffer_stream(
    std::shared_ptr<mc::BufferStream> const& stream) -> bool
{
    return streams.find(stream) != streams.end();
}

void ms::ApplicationSession::send_error(mir::ClientVisibleError const& error)
{
    event_sink->handle_error(error);
}
