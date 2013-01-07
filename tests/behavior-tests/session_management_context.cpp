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
#include "mir/frontend/consuming_placement_strategy.h"
#include "mir/frontend/placement_strategy_surface_organiser.h"
#include "mir/graphics/viewable_area.h"
#include "mir/server_configuration.h"

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

struct SizedBufferBundle : public mtd::NullBufferBundle
{
    SizedBufferBundle(geom::Size const& buffer_size) 
        : NullBufferBundle(),
          buffer_size(buffer_size)
    {
    }

    geometry::Size bundle_size()
    {
        return buffer_size;
    }

    geom::Size const buffer_size;
};

struct DummySurfaceOrganiser : public mf::SurfaceOrganiser
{
    explicit DummySurfaceOrganiser()
    {
    }

    std::weak_ptr<ms::Surface> create_surface(const ms::SurfaceCreationParameters& params)
    {
        auto name = params.name;
        auto surf = std::make_shared<ms::Surface>(name,
            std::make_shared<SizedBufferBundle>(params.size));
        surfaces_by_name[name] = surf;
        
        return surf;
    }

    void destroy_surface(std::weak_ptr<ms::Surface> const& /*surface*/)
    {
        // TODO: Implement
    }
    
    void hide_surface(std::weak_ptr<ms::Surface> const& /*surface*/)
    {
    }

    void show_surface(std::weak_ptr<ms::Surface> const& /*surface*/)
    {
    }
    
    std::map<std::string, std::shared_ptr<ms::Surface>> surfaces_by_name;
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
} // namespace mir

mtc::SessionManagementContext::SessionManagementContext()
{
    auto server_configuration = std::make_shared<mir::DefaultServerConfiguration>("" /* socket */);
    auto underlying_organiser = std::make_shared<mtc::DummySurfaceOrganiser>();
    view_area = std::make_shared<DummyViewableArea>();
    
    session_manager = server_configuration->make_session_manager(underlying_organiser, view_area);
}

// TODO: This will be less awkward with the ApplicationWindow class.
bool mtc::SessionManagementContext::open_window_consuming(std::string const& window_name)
{
    auto const params = ms::a_surface().of_name(window_name);
    auto session = session_manager->open_session(window_name);
    auto const surface_id = session->create_surface(params);

    open_windows[window_name] = std::make_tuple(session, surface_id);

    return true;
}

bool mtc::SessionManagementContext::open_window_sized(std::string const& window_name,
                                                      geom::Size const& size)
{
    auto const params = ms::a_surface().of_name(window_name).of_size(size);
    auto session = session_manager->open_session(window_name);
    auto const surface_id = session->create_surface(params);

    open_windows[window_name] = std::make_tuple(session, surface_id);

    return true;
}

geom::Size mtc::SessionManagementContext::get_window_size(std::string const& window_name) const
{
    auto const window = open_windows[window_name];
    auto const session = std::get<0>(window);
    auto const surface_id = std::get<1>(window);
    
    return session->get_surface(surface_id)->size();
}

void mtc::SessionManagementContext::set_view_area(geom::Rectangle const& new_view_region)
{
    view_area->set_view_area(new_view_region);
}
