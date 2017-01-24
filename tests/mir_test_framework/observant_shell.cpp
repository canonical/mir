/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir_test_framework/observant_shell.h"

namespace geom = mir::geometry;
namespace msh = mir::shell;
namespace msc = mir::scene;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace mtf = mir_test_framework;

mtf::ObservantShell::ObservantShell(
    std::shared_ptr<msh::Shell> const& wrapped,
    std::shared_ptr<msc::SurfaceObserver> const& surface_observer) :
    wrapped(wrapped),
    surface_observer(surface_observer)
{
}

void mtf::ObservantShell::add_display(geom::Rectangle const& area) 
{
    return wrapped->add_display(area);
}

void mtf::ObservantShell::remove_display(geom::Rectangle const& area) 
{
    return wrapped->remove_display(area);
}

bool mtf::ObservantShell::handle(MirEvent const& event) 
{
    return wrapped->handle(event);
}

void mtf::ObservantShell::focus_next_session() 
{
    wrapped->focus_next_session();
}

auto mtf::ObservantShell::focused_session() const -> std::shared_ptr<msc::Session> 
{
    return wrapped->focused_session();
}

void mtf::ObservantShell::set_focus_to(
    std::shared_ptr<msc::Session> const& focus_session,
    std::shared_ptr<msc::Surface> const& focus_surface) 
{
    return wrapped->set_focus_to(focus_session, focus_surface);
}

std::shared_ptr<msc::Surface> mtf::ObservantShell::focused_surface() const 
{
    return wrapped->focused_surface();
}

auto mtf::ObservantShell::surface_at(geom::Point cursor) const -> std::shared_ptr<msc::Surface> 
{
    return wrapped->surface_at(cursor);
}

void mtf::ObservantShell::raise(msh::SurfaceSet const& surfaces) 
{
    wrapped->raise(surfaces);
}

std::shared_ptr<msc::Session> mtf::ObservantShell::open_session(
    pid_t client_pid,
    std::string const& name,
    std::shared_ptr<mf::EventSink> const& sink) 
{
    return wrapped->open_session(client_pid, name, sink);
}

void mtf::ObservantShell::close_session(std::shared_ptr<msc::Session> const& session) 
{
    wrapped->close_session(session);
}

std::shared_ptr<msc::PromptSession> mtf::ObservantShell::start_prompt_session_for(
    std::shared_ptr<msc::Session> const& session,
    msc::PromptSessionCreationParameters const& params) 
{
    return wrapped->start_prompt_session_for(session, params);
}

void mtf::ObservantShell::add_prompt_provider_for(
    std::shared_ptr<msc::PromptSession> const& prompt_session,
    std::shared_ptr<msc::Session> const& session) 
{
    wrapped->add_prompt_provider_for(prompt_session, session);
}

void mtf::ObservantShell::stop_prompt_session(std::shared_ptr<msc::PromptSession> const& prompt_session) 
{
    wrapped->stop_prompt_session(prompt_session);
}

mf::SurfaceId mtf::ObservantShell::create_surface(
    std::shared_ptr<msc::Session> const& session,
    msc::SurfaceCreationParameters const& params,
    std::shared_ptr<mf::EventSink> const& sink) 
{
    auto id = wrapped->create_surface(session, params, sink);
    auto window = session->surface(id);
    window->add_observer(surface_observer);
    return id;
}

void mtf::ObservantShell::modify_surface(
    std::shared_ptr<msc::Session> const& session,
    std::shared_ptr<msc::Surface> const& window,
    msh::SurfaceSpecification  const& modifications) 
{
    wrapped->modify_surface(session, window, modifications);
}

void mtf::ObservantShell::destroy_surface(std::shared_ptr<msc::Session> const& session, mf::SurfaceId window) 
{
    wrapped->destroy_surface(session, window);
}

int mtf::ObservantShell::set_surface_attribute(
    std::shared_ptr<msc::Session> const& session,
    std::shared_ptr<msc::Surface> const& window,
    MirWindowAttrib attrib,
    int value) 
{
    return wrapped->set_surface_attribute(session, window, attrib, value);
}

int mtf::ObservantShell::get_surface_attribute(
    std::shared_ptr<msc::Surface> const& window,
    MirWindowAttrib attrib) 
{
    return wrapped->get_surface_attribute(window, attrib);
}

void mtf::ObservantShell::raise_surface(
    std::shared_ptr<msc::Session> const& session,
    std::shared_ptr<msc::Surface> const& window,
    uint64_t timestamp) 
{
    return wrapped->raise_surface(session, window, timestamp);
}
