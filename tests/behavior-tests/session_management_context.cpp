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
#include "mir/graphics/viewable_area.h"

namespace mf = mir::frontend;
namespace ms = mir::surfaces;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtc = mt::cucumber;
namespace mtd = mt::doubles;

namespace mir
{
namespace test
{
namespace cucumber
{

static const geom::Rectangle default_view_area = geom::Rectangle{geom::Point(),
                                                                 geom::Size{geom::Width(1600),
                                                                            geom::Height(1400)}};

struct StubSurfaceOrganiser : public mf::SurfaceOrganiser
{
    explicit StubSurfaceOrganiser()
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

struct DummyViewableArea : public mg::ViewableArea
{
    explicit DummyViewableArea(geom::Rectangle const& view_area = default_view_area)
        : area(view_area)
    {
    }
    
    geom::Rectangle view_area() const
    {
        return area;
    }
    
    void set_view_area(geom::Rectangle const& new_view_area)
    {
        area = new_view_area;
    }
    
    geom::Rectangle area;
};

}
}
}

mtc::SessionManagementContext::SessionManagementContext()
{
    // TODO: This should use a method from server configuration to construct session manager 
    // to ensure code stays in sync.
    auto model = std::make_shared<mf::SessionContainer>();
    session_manager = std::make_shared<mf::SessionManager>(
            std::make_shared<mtc::StubSurfaceOrganiser>(),
            model,
            std::make_shared<mf::RegistrationOrderFocusSequence>(model),
            std::make_shared<mf::SingleVisibilityFocusMechanism>(model));
    
    // TODO: Needs to be passed to the placement strategy when this is implemented
    view_area = std::make_shared<DummyViewableArea>();
}

// TODO: This would be less awkward with the ApplicationWindow class.
bool mtc::SessionManagementContext::open_window_consuming(std::string const& window_name)
{
    auto params = ms::a_surface().of_name(window_name);
    auto session = session_manager->open_session(window_name);

    open_windows[window_name] = std::make_tuple(session, session->create_surface(params));

    return true;
}

geom::Size mtc::SessionManagementContext::get_window_size(std::string const& window_name)
{
    auto window = open_windows[window_name];
    auto session = std::get<0>(window);
    auto surface_id = std::get<1>(window);
    
    return session->get_surface(surface_id)->size();
}

void mtc::SessionManagementContext::set_view_area(geom::Rectangle const& new_view_region)
{
    view_area->set_view_area(new_view_region);
}
