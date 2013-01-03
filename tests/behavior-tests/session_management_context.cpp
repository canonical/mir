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

#include "mir_test_doubles/null_buffer_bundle.h"

#include "mir/surfaces/surface.h"
#include "mir/frontend/registration_order_focus_sequence.h"
#include "mir/frontend/single_visibility_focus_mechanism.h"
#include "mir/frontend/session_container.h"
#include "mir/frontend/session.h"
#include "mir/frontend/session_manager.h"
#include "mir/frontend/surface_organiser.h"

namespace mf = mir::frontend;
namespace ms = mir::surfaces;
namespace mt = mir::test;
namespace mtc = mt::cucumber;
namespace mtd = mt::doubles;

namespace
{

struct StubSurfaceOrganiser : public mf::SurfaceOrganiser
{
    StubSurfaceOrganiser()
    {
        // TODO: Width and height will require a non null buffer bundle...
        dummy_surface = std::make_shared<ms::Surface>(ms::a_surface().name,
                                                      std::make_shared<mtd::NullBufferBundle>());
    }

    std::weak_ptr<ms::Surface> create_surface(const ms::SurfaceCreationParameters& /*params*/)
    {
        return dummy_surface;
    }

    void destroy_surface(std::weak_ptr<ms::Surface> const& /*surface*/)
    {
    }
    
    void hide_surface(std::weak_ptr<ms::Surface> const& /*surface*/)
    {
    }

    void show_surface(std::weak_ptr<ms::Surface> const& /*surface*/)
    {
    }
    
    std::shared_ptr<ms::Surface> dummy_surface;
};

}

mtc::SessionManagementContext::SessionManagementContext()
{
    auto model = std::make_shared<mf::SessionContainer>();
    session_manager = std::make_shared<mf::SessionManager>(
            std::make_shared<StubSurfaceOrganiser>(),
            model,
            std::make_shared<mf::RegistrationOrderFocusSequence>(model),
            std::make_shared<mf::SingleVisibilityFocusMechanism>(model));
}

bool mtc::SessionManagementContext::open_session(const std::string& session_name)
{
    open_sessions[session_name] = session_manager->open_session(session_name);
    return true;
}
