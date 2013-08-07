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
 * Authored By: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/frontend/session.h"
#include "mir/shell/default_focus_mechanism.h"
#include "mir/shell/input_targeter.h"
#include "mir/shell/session.h"
#include "mir/shell/surface.h"
#include "mir/shell/surface_controller.h"
#include "mir/shell/session_listener.h"
#include "mir/shell/focus_sequence.h"

namespace mf = mir::frontend;
namespace msh = mir::shell;

msh::DefaultFocusMechanism::DefaultFocusMechanism(std::shared_ptr<msh::FocusSequence> const& sequence,
                                                  std::shared_ptr<msh::InputTargeter> const& input_targeter,
                                                  std::shared_ptr<msh::SurfaceController> const& surface_controller,
                                                  std::shared_ptr<msh::SessionListener> const& session_listener)
  : sequence(sequence),
    session_listener(session_listener),
    input_targeter(input_targeter),
    surface_controller(surface_controller),
    focus_session(nullptr)
{ 
}

void msh::DefaultFocusMechanism::set_focus(std::shared_ptr<msh::Session> const& session)
{
    focus_session = session;
    session_listener->focused(focus_session);

    auto surface = focus_session->default_surface();
    if (surface)
    {
        surface->raise(surface_controller);
        surface->take_input_focus(input_targeter);
    } 

}

void msh::DefaultFocusMechanism::surface_created_for(std::shared_ptr<Session> const& session)
{
    std::unique_lock<std::mutex> lock(mutex);

    //just set focus when a new surface is created
    set_focus(session);
}
void msh::DefaultFocusMechanism::session_opened(std::shared_ptr<Session> const& session)
{
    std::unique_lock<std::mutex> lock(mutex);

    //just set focus to new session for now
    set_focus(session);
}

void msh::DefaultFocusMechanism::session_closed(std::shared_ptr<Session> const& session)
{
    std::unique_lock<std::mutex> lock(mutex);

    if (focus_session == session)
    {
        //cycle through to next focus if the currenttly-focused app is closed
        try
        {
            focus_session = sequence->successor_of(focus_session);
            set_focus(session);
        } catch (std::logic_error)
        {
            input_targeter->focus_cleared();
            session_listener->unfocused();
        } 
    }
}

void msh::DefaultFocusMechanism::focus_next()
{
#if 0
    std::unique_lock<std::mutex> lock(mutex);
    //todo, should be in constructor, small dependency between sequence and the session manager to sort out
    if(!focus_session)
        focus_session = sequence->default_focus();
    else
        focus_session = sequence->successor_of(focus_session);

    display_changer->set_focus_to(focus_session);
    session_listener->focused(focus_session);

    //todo shouldnt do this here
    auto surface = focus_session->default_surface();
    if (surface)
    {
        std::cout << " focus change " << surface << std::endl;
        printf("RAISIN \n");
        surface->raise(surface_controller);
        surface->take_input_focus(input_targeter);
    } 
#endif
}

std::weak_ptr<msh::Session> msh::DefaultFocusMechanism::focused_session() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return focus_session;
}
