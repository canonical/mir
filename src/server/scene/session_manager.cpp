/*
 * Copyright © 2012-2019 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "session_manager.h"
#include "application_session.h"
#include "mir/scene/session_container.h"
#include "mir/scene/surface.h"
#include "mir/scene/session.h"
#include "mir/scene/session_listener.h"
#include "mir/scene/prompt_session.h"
#include "mir/scene/application_not_responding_detector.h"
#include "mir/shell/surface_stack.h"
#include "mir/scene/session_event_sink.h"
#include "mir/frontend/event_sink.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_configuration.h"
#include "mir/observer_multiplexer.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <memory>
#include <cassert>
#include <algorithm>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace msh = mir::shell;

struct ms::SessionManager::SessionObservers : mir::Executor, mir::ObserverMultiplexer<ms::SessionListener>
{
    SessionObservers()
        : ObserverMultiplexer<ms::SessionListener>(static_cast<Executor&>(*this))
    {
    }
    void spawn(std::function<void()>&& work) override
    {
        work();
    }

    void starting(std::shared_ptr<Session> const& session) override
    {
        for_each_observer(&SessionListener::starting, session);
    }
    void stopping(std::shared_ptr<Session> const& session) override
    {
        for_each_observer(&SessionListener::stopping, session);
    }
    void focused(std::shared_ptr<Session> const& session) override
    {
        for_each_observer(&SessionListener::focused, session);
    }
    void unfocused() override
    {
        for_each_observer(&SessionListener::unfocused);
    }
    void surface_created(Session& session, std::shared_ptr<Surface> const& surface) override
    {
        for_each_observer(&SessionListener::surface_created, std::ref(session), surface);
    }
    void destroying_surface(Session& session, std::shared_ptr<Surface> const& surface) override
    {
        for_each_observer(&SessionListener::destroying_surface, std::ref(session), surface);
    }
    void buffer_stream_created(Session& session, std::shared_ptr<mf::BufferStream> const& stream) override
    {
        for_each_observer(&SessionListener::buffer_stream_created, std::ref(session), stream);
    }
    void buffer_stream_destroyed(Session& session, std::shared_ptr<mf::BufferStream> const& stream) override
    {
        for_each_observer(&SessionListener::buffer_stream_destroyed, std::ref(session), stream);
    }
};

ms::SessionManager::SessionManager(
    std::shared_ptr<shell::SurfaceStack> const& surface_stack,
    std::shared_ptr<SurfaceFactory> const& surface_factory,
    std::shared_ptr<BufferStreamFactory> const& buffer_stream_factory,
    std::shared_ptr<SessionContainer> const& container,
    std::shared_ptr<SnapshotStrategy> const& snapshot_strategy,
    std::shared_ptr<SessionEventSink> const& session_event_sink,
    std::shared_ptr<SessionListener> const& session_listener,
    std::shared_ptr<graphics::Display const> const& display,
    std::shared_ptr<ApplicationNotRespondingDetector> const& anr_detector,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> display_config_registrar) :
    observers(std::make_shared<SessionObservers>()),
    surface_stack(surface_stack),
    surface_factory(surface_factory),
    buffer_stream_factory(buffer_stream_factory),
    app_container(container),
    snapshot_strategy(snapshot_strategy),
    session_event_sink(session_event_sink),
    session_listener(session_listener),
    display{display},
    anr_detector{anr_detector},
    allocator{allocator},
    display_config_registrar{display_config_registrar}
{
    observers->register_interest(session_listener);
}

ms::SessionManager::~SessionManager() noexcept
{
    /*
     * Close all open sessions. We need to do this manually here
     * to break the cyclic dependency between msh::Session
     * and mi::*, since our implementations
     * of these interfaces keep strong references to each other.
     * TODO: Investigate other solutions (e.g. weak_ptr)
     */
    std::vector<std::shared_ptr<Session>> sessions;

    app_container->for_each([&](std::shared_ptr<Session> const& session)
    {
        sessions.push_back(session);
    });

    for (auto& session : sessions)
        close_session(session);
}

std::shared_ptr<ms::Session> ms::SessionManager::open_session(
    pid_t client_pid,
    std::string const& name,
    std::shared_ptr<mf::EventSink> const& sender)
{
    std::shared_ptr<Session> new_session = std::make_shared<ApplicationSession>(
        surface_stack,
            surface_factory,
            buffer_stream_factory,
            client_pid,
            name,
            snapshot_strategy,
            observers,
            *display->configuration(),
            sender,
            allocator,
            display_config_registrar);

    app_container->insert_session(new_session);

    observers->starting(new_session);

    anr_detector->register_session(new_session.get(), [sender]()
    {
        sender->send_ping(0);
    });

    return new_session;
}

void ms::SessionManager::set_focus_to(std::shared_ptr<Session> const& session)
{
    session_event_sink->handle_focus_change(session);
    observers->focused(session);
}

void ms::SessionManager::unset_focus()
{
    session_event_sink->handle_no_focus();
    observers->unfocused();
}

void ms::SessionManager::close_session(std::shared_ptr<Session> const& session)
{
    anr_detector->unregister_session(session.get());

    session_event_sink->handle_session_stopping(session);

    observers->stopping(session);

    app_container->remove_session(session);
}


auto ms::SessionManager::successor_of(std::shared_ptr<Session> const& session) const
    -> std::shared_ptr<ms::Session>
{
    return app_container->successor_of(session);
}

auto ms::SessionManager::predecessor_of(std::shared_ptr<Session> const& session) const
    -> std::shared_ptr<mir::scene::Session>
{
    return app_container->predecessor_of(session);
}

void ms::SessionManager::add_listener(std::shared_ptr<SessionListener> const& listener)
{
    observers->register_interest(listener);
}

void ms::SessionManager::remove_listener(std::shared_ptr<SessionListener> const& listener)
{
    observers->unregister_interest(*listener);
}
