/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 * Authored By: William Wold <william.wold@canonical.com>
 */

#include "mir/frontend/basic_mir_client_session.h"

#include "mir/shell/shell.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_event_source.h"
#include "mir/graphics/display_configuration_observer.h"
#include "mir/events/event_builders.h"
#include "mir/executor.h"
#include "mir/compositor/buffer_stream.h"

#include <boost/throw_exception.hpp>
#include <algorithm>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mev = mir::events;

class mf::BasicMirClientSession::DisplayConfigurationObserver
    : public graphics::DisplayConfigurationObserver
{
public:
    DisplayConfigurationObserver(BasicMirClientSession* session)
        : session{session}
    {
    }

    void initial_configuration(std::shared_ptr<mg::DisplayConfiguration const> const&) override
    {
    }

    void configuration_applied(std::shared_ptr<mg::DisplayConfiguration const> const&) override
    {
    }

    void base_configuration_updated(std::shared_ptr<mg::DisplayConfiguration const> const&) override
    {
    }

    void session_configuration_applied(
        std::shared_ptr<scene::Session> const&,
        std::shared_ptr<mg::DisplayConfiguration> const&) override
    {
    }

    void session_configuration_removed(std::shared_ptr<scene::Session> const&) override
    {
    }

    void configuration_failed(
        std::shared_ptr<mg::DisplayConfiguration const> const&,
        std::exception const&) override
    {
    }

    void catastrophic_configuration_error(
        std::shared_ptr<mg::DisplayConfiguration const> const&,
        std::exception const&) override
    {
    }

    void configuration_updated_for_session(
        std::shared_ptr<ms::Session> const& session,
        std::shared_ptr<mg::DisplayConfiguration const> const& config) override
    {
        if (session == this->session->session_)
            this->session->send_display_config(*config);
    }

private:
    BasicMirClientSession* const session;
};

mf::BasicMirClientSession::BasicMirClientSession(
    std::shared_ptr<scene::Session> session,
    std::shared_ptr<frontend::EventSink> const& event_sink,
    graphics::DisplayConfiguration const& initial_display_config,
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& display_config_registrar)
    : session_{session},
      event_sink{event_sink},
      display_config_registrar{display_config_registrar},
      display_config_observer{std::make_shared<DisplayConfigurationObserver>(this)},
      next_id{0}
{
    output_cache.update_from(initial_display_config);
    display_config_registrar->register_interest(display_config_observer);
}

mf::BasicMirClientSession::~BasicMirClientSession()
{
    if (auto const registrar = display_config_registrar.lock())
        registrar->unregister_interest(*display_config_observer);
}

auto mf::BasicMirClientSession::name() const -> std::string
{
    return session_->name();
}

auto mf::BasicMirClientSession::frontend_surface(SurfaceId id) const -> std::shared_ptr<Surface>
{
    return std::dynamic_pointer_cast<Surface>(scene_surface(id));
}

auto mf::BasicMirClientSession::scene_surface(SurfaceId id) const -> std::shared_ptr<ms::Surface>
{
    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);

    auto iter = checked_find(id);

    if (auto const surface = iter->second.lock())
        return surface;
    else
        BOOST_THROW_EXCEPTION(std::logic_error(
            "Surface dropped without being removed from its MirClientSession"));
}

auto mf::BasicMirClientSession::create_surface(
    std::shared_ptr<shell::Shell> const& shell,
    scene::SurfaceCreationParameters const& params,
    std::shared_ptr<frontend::EventSink> const& sink) -> frontend::SurfaceId
{
    auto const id = mf::SurfaceId(next_id.fetch_add(1));
    auto const event_source = std::make_shared<scene::SurfaceEventSource>(
        id,
        output_cache,
        sink);
    auto const surface = shell->create_surface(session_, params, event_source);

    {
        std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
        surfaces[id] = surface;
    }

    return id;
}

void mf::BasicMirClientSession::destroy_surface(
    std::shared_ptr<shell::Shell> const& shell,
    frontend::SurfaceId id)
{
    auto const surface = scene_surface(id);

    {
        std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
        surfaces.erase(id);
    }

    shell->destroy_surface(session_, surface);
}

auto mf::BasicMirClientSession::create_buffer_stream(graphics::BufferProperties const& props) -> BufferStreamId
{
    auto const id = mf::BufferStreamId(next_id.fetch_add(1));
    auto const stream = session_->create_buffer_stream(props);

    {
        std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
        streams[id] = stream;
    }

    return id;
}

auto mf::BasicMirClientSession::buffer_stream(BufferStreamId id) const -> std::shared_ptr<compositor::BufferStream>
{
    std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);

    auto iter = checked_find(id);

    if (auto const stream = iter->second.lock())
        return stream;
    else
        BOOST_THROW_EXCEPTION(std::logic_error(
            "Surface dropped without being removed from its MirClientSession"));
}

void mf::BasicMirClientSession::destroy_buffer_stream(BufferStreamId id)
{
    auto const stream = buffer_stream(id);

    {
        std::unique_lock<std::mutex> lock(surfaces_and_streams_mutex);
        streams.erase(id);
    }

    session_->destroy_buffer_stream(stream);
}

void mf::BasicMirClientSession::send_display_config(mg::DisplayConfiguration const& info)
{
    output_cache.update_from(info);

    event_sink->handle_display_config_change(info);

    std::lock_guard<std::mutex> lock{surfaces_and_streams_mutex};
    for (auto& iter : surfaces)
    {
        if (auto const surface = iter.second.lock())
        {
            auto output_properties = output_cache.properties_for(geometry::Rectangle{
                surface->top_left(),
                surface->window_size()});

            if (output_properties)
            {
                event_sink->handle_event(
                    mev::make_event(
                        iter.first,
                        output_properties->dpi,
                        output_properties->scale,
                        output_properties->refresh_rate,
                        output_properties->form_factor,
                        static_cast<uint32_t>(output_properties->id.as_value())
                        ));
            }
        }
    }
}

auto mf::BasicMirClientSession::checked_find(frontend::SurfaceId id) const -> Surfaces::const_iterator
{
    auto iter = surfaces.find(id);
    if (iter == surfaces.end())
        BOOST_THROW_EXCEPTION(std::out_of_range("Invalid surface ID"));
    else
        return iter;
}

auto mf::BasicMirClientSession::checked_find(
    std::shared_ptr<scene::Surface> const& surface) const -> Surfaces::const_iterator
{
    auto iter = std::find_if(surfaces.begin(), surfaces.end(), [&](auto i)
        {
            return i.second.lock() == surface;
        });
    if (iter == surfaces.end())
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid Surface"));
    else
        return iter;
}

auto mf::BasicMirClientSession::checked_find(frontend::BufferStreamId id) const -> Streams::const_iterator
{
    auto iter = streams.find(id);
    if (iter == streams.end())
        BOOST_THROW_EXCEPTION(std::out_of_range("Invalid stream ID"));
    else
        return iter;
}
