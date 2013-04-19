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

#include "mir_test_cucumber/session_management_context.h"

#include "mir/frontend/surface.h"
#include "mir/frontend/surface_creation_parameters.h"
#include "mir/frontend/session.h"
#include "mir/shell/registration_order_focus_sequence.h"
#include "mir/shell/single_visibility_focus_mechanism.h"
#include "mir/shell/session_container.h"
#include "mir/frontend/shell.h"
#include "mir/input/input_channel.h"
#include "mir/shell/surface_factory.h"
#include "mir/graphics/display.h"
#include "mir/default_server_configuration.h"

#include "mir/shell/surface.h"

#include "mir_test_doubles/stub_surface_builder.h"
#include "mir_test/fake_shared.h"

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtc = mir::test::cucumber;
namespace mtd = mir::test::doubles;

namespace mir
{
namespace test
{
namespace cucumber
{

static const geom::Width default_view_width = geom::Width{1600};
static const geom::Height default_view_height = geom::Height{1400};

static const geom::Size default_view_size = geom::Size{default_view_width,
                                                       default_view_height};
static const geom::Rectangle default_view_area = geom::Rectangle{geom::Point(),
                                                                 default_view_size};

struct DummySurfaceFactory : public msh::SurfaceFactory
{
    explicit DummySurfaceFactory()
    {
    }

    std::shared_ptr<msh::Surface> create_surface(const mf::SurfaceCreationParameters& params)
    {
        return std::make_shared<msh::Surface>(
            mt::fake_shared(surface_builder),
            params,
            std::shared_ptr<mir::input::InputChannel>());
    }

    mtd::StubSurfaceBuilder surface_builder;
};

class SizedDisplay : public mg::Display
{
public:
    explicit SizedDisplay(geom::Rectangle const& view_area = default_view_area)
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

    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
    {
        (void)f;
    }

    std::shared_ptr<mg::DisplayConfiguration> configuration()
    {
        return std::shared_ptr<mg::DisplayConfiguration>();
    }

    void register_pause_resume_handlers(MainLoop&,
                                        std::function<void()> const&,
                                        std::function<void()> const&)
    {
    }

    void pause() {}
    void resume() {}

    geom::Rectangle area;
};

}
}
} // namespace mir

namespace
{
    struct DummyServerConfiguration : mir::DefaultServerConfiguration
    {
        DummyServerConfiguration() : mir::DefaultServerConfiguration(0, nullptr) {}

        virtual std::shared_ptr<mir::shell::SurfaceFactory>
        the_shell_surface_factory()
        {
            return std::make_shared<mtc::DummySurfaceFactory>();
        }
    } server_configuration;
}

mtc::SessionManagementContext::SessionManagementContext() :
    view_area(std::make_shared<mtc::SizedDisplay>()),
    shell(server_configuration.the_frontend_shell())
{
}

mtc::SessionManagementContext::SessionManagementContext(ServerConfiguration& server_configuration) :
    view_area(std::make_shared<mtc::SizedDisplay>()),
    shell(server_configuration.the_frontend_shell())
{
}

// TODO: This will be less awkward with the ApplicationWindow class.
bool mtc::SessionManagementContext::open_window_consuming(std::string const& window_name)
{
    auto const params = mf::a_surface().of_name(window_name);
    auto session = shell->open_session(window_name);
    auto const surface_id = session->create_surface(params);

    open_windows[window_name] = std::make_tuple(session, surface_id);

    return true;
}

bool mtc::SessionManagementContext::open_window_with_size(std::string const& window_name,
                                                          geom::Size const& size)
{
    auto const params = mf::a_surface().of_name(window_name).of_size(size);
    auto session = shell->open_session(window_name);
    auto const surface_id = session->create_surface(params);

    open_windows[window_name] = std::make_tuple(session, surface_id);

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

std::shared_ptr<mg::ViewableArea> mtc::SessionManagementContext::get_view_area() const
{
    return view_area;
}
