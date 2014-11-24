/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/command_line_server_configuration.h"

#include "stubbed_graphics_platform.h"

#include "mir/options/default_configuration.h"
#include "mir/graphics/cursor.h"

#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/stub_renderer.h"
#include "mir_test_doubles/stub_input_sender.h"

#include "mir/compositor/renderer_factory.h"
#include "src/server/input/null_input_manager.h"
#include "src/server/input/null_input_dispatcher.h"
#include "src/server/input/null_input_targeter.h"
#include "mir_test_doubles/null_logger.h"

namespace geom = mir::geometry;
namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace ml = mir::logging;
namespace mo = mir::options;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{
class StubCursor : public mg::Cursor
{
    void show(mg::CursorImage const&) override {}
    void hide() override {}
    void move_to(geom::Point) override {}
};

class StubRendererFactory : public mc::RendererFactory
{
public:
    std::unique_ptr<mc::Renderer> create_renderer_for(geom::Rectangle const&, mc::DestinationAlpha)
    {
        return std::unique_ptr<mc::Renderer>(new mtd::StubRenderer());
    }
};

}

mtf::StubbedServerConfiguration::StubbedServerConfiguration()
    : StubbedServerConfiguration({geom::Rectangle{{0,0},{1600,1600}}})
{
}

mtf::StubbedServerConfiguration::StubbedServerConfiguration(
    std::vector<geom::Rectangle> const& display_rects)
    : DefaultServerConfiguration([]
      {
          auto result = mtf::configuration_from_commandline();

          namespace po = boost::program_options;

          result->add_options()
                  (mtd::logging_opt, po::value<bool>()->default_value(false), mtd::logging_descr)
                  ("tests-use-real-graphics", po::value<bool>()->default_value(false), "Use real graphics in tests.")
                  ("tests-use-real-input", po::value<bool>()->default_value(false), "Use real input in tests.");

          return result;
      }()),
      display_rects{display_rects}
{
}

std::shared_ptr<mg::Platform> mtf::StubbedServerConfiguration::the_graphics_platform()
{
    if (!graphics_platform)
    {
        graphics_platform = std::make_shared<StubGraphicPlatform>(display_rects);
    }

    return graphics_platform;
}

std::shared_ptr<mc::RendererFactory> mtf::StubbedServerConfiguration::the_renderer_factory()
{
    auto options = the_options();

    if (options->get<bool>("tests-use-real-graphics"))
        return DefaultServerConfiguration::the_renderer_factory();
    else
        return renderer_factory(
            [&]()
            {
                return std::make_shared<StubRendererFactory>();
            });
}

std::shared_ptr<mi::InputManager> mtf::StubbedServerConfiguration::the_input_manager()
{
    auto options = the_options();

    if (options->get<bool>("tests-use-real-input"))
        return DefaultServerConfiguration::the_input_manager();
    else
        return std::make_shared<mi::NullInputManager>();
}

std::shared_ptr<msh::InputTargeter> mtf::StubbedServerConfiguration::the_input_targeter()
{
    auto options = the_options();

    if (options->get<bool>("tests-use-real-input"))
        return DefaultServerConfiguration::the_input_targeter();
    else
        return std::make_shared<mi::NullInputTargeter>();
}

std::shared_ptr<mi::InputDispatcher> mtf::StubbedServerConfiguration::the_input_dispatcher()
{
    auto options = the_options();

    if (options->get<bool>("tests-use-real-input"))
        return DefaultServerConfiguration::the_input_dispatcher();
    else
        return std::make_shared<mi::NullInputDispatcher>();
}

std::shared_ptr<mi::InputSender> mtf::StubbedServerConfiguration::the_input_sender()
{
    auto options = the_options();

    if (options->get<bool>("tests-use-real-input"))
        return DefaultServerConfiguration::the_input_sender();
    else
        return std::make_shared<mtd::StubInputSender>();
}

std::shared_ptr<mg::Cursor> mtf::StubbedServerConfiguration::the_cursor()
{
    return std::make_shared<StubCursor>();
}

std::shared_ptr<ml::Logger> mtf::StubbedServerConfiguration::the_logger()
{
    if (the_options()->get<bool>(mtd::logging_opt))
        return DefaultServerConfiguration::the_logger();

    return std::make_shared<mtd::NullLogger>();
}
