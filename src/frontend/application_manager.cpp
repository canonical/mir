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

#include "mir/frontend/application_manager.h"
#include "mir/frontend/application_session.h"
#include "mir/frontend/application_session_container.h"
#include "mir/surfaces/application_surface_organiser.h"
#include "mir/frontend/application_focus_selection_strategy.h"
#include "mir/frontend/application_focus_mechanism.h"

#include <memory>
#include <cassert>
#include <algorithm>

namespace mf = mir::frontend;
namespace ms = mir::surfaces;

mf::ApplicationManager::ApplicationManager(std::shared_ptr<ms::ApplicationSurfaceOrganiser> organiser, 
                                           std::shared_ptr<mf::ApplicationSessionContainer> container,
                                           std::shared_ptr<mf::ApplicationFocusSelectionStrategy> strategy,
                                           std::shared_ptr<mf::ApplicationFocusMechanism> mechanism) : surface_organiser(organiser),
                                                                                                       app_container(container),
                                                                                                       focus_selection_strategy(strategy),
                                                                                                       focus_mechanism(mechanism)
															     
{
    assert(surface_organiser);
    assert(strategy);
    assert(container);
    assert(mechanism);
}

std::shared_ptr<mf::ApplicationSession> mf::ApplicationManager::open_session(std::string name)
{
    auto new_session = std::make_shared<mf::ApplicationSession>(surface_organiser, name);
  
    app_container->insert_session(new_session);
    focus_application = new_session;
    focus_mechanism->set_focus_to(new_session);
  
    return new_session;
}

void mf::ApplicationManager::close_session(std::shared_ptr<mf::ApplicationSession> session)
{
    if (session == focus_application.lock())
    {
        focus_application = focus_selection_strategy->previous_focus_app(app_container, session);
        focus_mechanism->set_focus_to(focus_application.lock());
    }
    app_container->remove_session(session);
}

void mf::ApplicationManager::focus_next()
{
    auto focused = focus_application.lock();
    if (focused == NULL)
    {
        return;
    }
    auto next_focus = focus_selection_strategy->next_focus_app(app_container, focused).lock();
    focus_application = next_focus;
    focus_mechanism->set_focus_to(next_focus);
}
