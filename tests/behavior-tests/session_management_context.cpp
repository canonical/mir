/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "session_management_context.h"

#include "mir/sessions/registration_order_focus_sequence.h"
#include "mir/sessions/single_visibility_focus_mechanism.h"
#include "mir/sessions/session_container.h"
#include "mir/sessions/session_manager.h"
#include "mir/sessions/surface_factory.h"

namespace msess = mir::sessions;
namespace mtc = mir::test::cucumber;

namespace
{

struct StubSurfaceFactory : public msess::SurfaceFactory
{
    StubSurfaceFactory()
    {
        // TODO: Width and height will require a non null buffer bundle...
        null_surface = std::shared_ptr<msess::Surface>();
    }

    std::shared_ptr<msess::Surface> create_surface(const msess::SurfaceCreationParameters& /*params*/)
    {
        return null_surface;
    }

    std::shared_ptr<msess::Surface> null_surface;
};

}

mtc::SessionManagementContext::SessionManagementContext()
{
    auto model = std::make_shared<msess::SessionContainer>();
    session_manager = std::make_shared<msess::SessionManager>(
            std::make_shared<StubSurfaceFactory>(),
            model,
            std::make_shared<msess::RegistrationOrderFocusSequence>(model),
            std::make_shared<msess::SingleVisibilityFocusMechanism>(model));
}

bool mtc::SessionManagementContext::open_session(std::string const& session_name)
{
    open_sessions[session_name] = session_manager->open_session(session_name);
    return true;
}
