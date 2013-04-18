/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#include "mir/shell/session_manager.h"
#include "mir/shell/application_session.h"
#include "mir/shell/session_container.h"
#include "mir/shell/surface_factory.h"
#include "mir/shell/focus_sequence.h"
#include "mir/shell/focus_setter.h"
#include "mir/shell/session.h"
#include "mir/shell/surface.h"
#include "mir/shell/input_target_listener.h"

#include <memory>
#include <cassert>
#include <algorithm>

namespace mf = mir::frontend;
namespace msh = mir::shell;

msh::SessionManager::SessionManager(std::shared_ptr<msh::SurfaceFactory> const& surface_factory,
    std::shared_ptr<msh::SessionContainer> const& container,
    std::shared_ptr<msh::FocusSequence> const& sequence,
    std::shared_ptr<msh::FocusSetter> const& focus_setter,
    std::shared_ptr<msh::InputTargetListener> const& input_target_listener) :
    surface_factory(surface_factory),
    app_container(container),
    focus_sequence(sequence),
    focus_setter(focus_setter),
    input_target_listener(input_target_listener)
{
    assert(surface_factory);
    assert(sequence);
    assert(container);
    assert(focus_setter);
    assert(input_target_listener);
}

msh::SessionManager::~SessionManager()
{
}

std::shared_ptr<mf::Session> msh::SessionManager::open_session(std::string const& name)
{
    auto new_session = std::make_shared<msh::ApplicationSession>(surface_factory, input_target_listener, name);

    app_container->insert_session(new_session);
    
    input_target_listener->input_application_opened(new_session);

    set_focus_to_locked(std::unique_lock<std::mutex>(mutex), new_session);

    return new_session;
}

inline void msh::SessionManager::set_focus_to_locked(std::unique_lock<std::mutex> const&,
    std::shared_ptr<mf::Session> const& next_focus)
{
    auto shell_session = std::dynamic_pointer_cast<msh::Session>(next_focus);
    auto old_focus = focus_application.lock();

    focus_application = shell_session;
    focus_setter->set_focus_to(shell_session);

    if (shell_session && shell_session->default_surface())
        input_target_listener->focus_changed(shell_session->default_surface());
    else if (shell_session == old_focus || !shell_session)
        input_target_listener->focus_cleared();
}

void msh::SessionManager::close_session(std::shared_ptr<mf::Session> const& session)
{
    input_target_listener->input_application_closed(std::dynamic_pointer_cast<msh::Session>(session));
    app_container->remove_session(session);

    std::unique_lock<std::mutex> lock(mutex);
    set_focus_to_locked(lock, focus_sequence->default_focus());

    typedef Tags::value_type Pair;

    auto remove = std::remove_if(tags.begin(), tags.end(),
        [&](Pair const& v) { return v.second == session;});

    tags.erase(remove, tags.end());
}

void msh::SessionManager::focus_next()
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
    set_focus_to_locked(lock, focus);
}

void msh::SessionManager::force_requests_to_complete()
{
    app_container->for_each([](std::shared_ptr<mf::Session> const& session)
    {
        session->force_requests_to_complete();
    });
}

void msh::SessionManager::tag_session_with_lightdm_id(std::shared_ptr<mf::Session> const& session, int id)
{
    std::unique_lock<std::mutex> lock(mutex);
    typedef Tags::value_type Pair;

    auto remove = std::remove_if(tags.begin(), tags.end(),
        [&](Pair const& v) { return v.first == id || v.second == session;});

    tags.erase(remove, tags.end());

    tags.push_back(Pair(id, session));
}

void msh::SessionManager::focus_session_with_lightdm_id(int id)
{
    std::unique_lock<std::mutex> lock(mutex);
    typedef Tags::value_type Pair;

    auto match = std::find_if(tags.begin(), tags.end(),
        [&](Pair const& v) { return v.first == id; });

    if (tags.end() != match)
    {
        set_focus_to_locked(lock, match->second);
    }
}

mf::SurfaceId msh::SessionManager::create_surface_for(std::shared_ptr<mf::Session> const& session,
    mf::SurfaceCreationParameters const& params)
{
    auto id = session->create_surface(params);
    input_target_listener->input_surface_opened(std::dynamic_pointer_cast<msh::Session>(session),
        std::dynamic_pointer_cast<msh::Surface>(session->get_surface(id)));
    set_focus_to_locked(std::unique_lock<std::mutex>(mutex), session);

    return id;
}
