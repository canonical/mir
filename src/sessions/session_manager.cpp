/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/sessions/session_manager.h"
#include "mir/sessions/application_session.h"
#include "mir/sessions/session_container.h"
#include "mir/sessions/surface_factory.h"
#include "mir/sessions/focus_sequence.h"
#include "mir/sessions/focus_setter.h"

#include <memory>
#include <cassert>
#include <algorithm>

namespace msess = mir::sessions;

msess::SessionManager::SessionManager(
    std::shared_ptr<msess::SurfaceFactory> const& surface_factory,
    std::shared_ptr<msess::SessionContainer> const& container,
    std::shared_ptr<msess::FocusSequence> const& sequence,
    std::shared_ptr<msess::FocusSetter> const& focus_setter) :
    surface_factory(surface_factory),
    app_container(container),
    focus_sequence(sequence),
    focus_setter(focus_setter)
{
    assert(surface_factory);
    assert(sequence);
    assert(container);
    assert(focus_setter);
}

msess::SessionManager::~SessionManager()
{
}

std::shared_ptr<msess::Session> msess::SessionManager::open_session(std::string const& name)
{
    auto new_session = std::make_shared<msess::ApplicationSession>(surface_factory, name);

    app_container->insert_session(new_session);

    {
        std::unique_lock<std::mutex> lock(mutex);
        focus_application = new_session;
    }
    focus_setter->set_focus_to(new_session);
 
    return new_session;
}

inline void msess::SessionManager::set_focus_to(std::shared_ptr<Session> const& next_focus)
{
    focus_application = next_focus;
    focus_setter->set_focus_to(next_focus);
}

void msess::SessionManager::close_session(std::shared_ptr<msess::Session> const& session)
{
    std::unique_lock<std::mutex> lock(mutex);

    app_container->remove_session(session);
    set_focus_to(focus_sequence->default_focus());

    typedef Tags::value_type Pair;

    auto remove = std::remove_if(tags.begin(), tags.end(),
        [&](Pair const& v) { return v.second == session;});

    tags.erase(remove, tags.end());
}

void msess::SessionManager::focus_next()
{
    std::unique_lock<std::mutex> lock(mutex);
    auto focus = focus_application.lock();
    if (!focus)
    {
        focus = focus_sequence->default_focus();
    }
    else
    {
        focus = focus_sequence->successor_of(focus);
    }
    set_focus_to(focus);
}

void msess::SessionManager::shutdown()
{
    app_container->for_each([](std::shared_ptr<Session> const& session)
    {
        session->shutdown();
    });
}

void msess::SessionManager::tag_session_with_lightdm_id(std::shared_ptr<Session> const& session, int id)
{
    std::unique_lock<std::mutex> lock(mutex);
    typedef Tags::value_type Pair;

    auto remove = std::remove_if(tags.begin(), tags.end(),
        [&](Pair const& v) { return v.first == id || v.second == session;});

    tags.erase(remove, tags.end());

    tags.push_back(Pair(id, session));
}

void msess::SessionManager::focus_session_with_lightdm_id(int id)
{
    std::unique_lock<std::mutex> lock(mutex);
    typedef Tags::value_type Pair;

    auto match = std::find_if(tags.begin(), tags.end(),
        [&](Pair const& v) { return v.first == id; });

    if (tags.end() != match)
    {
        set_focus_to(match->second);
    }
}
