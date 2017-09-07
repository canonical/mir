/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "mir_test_framework/stub_server_platform_factory.h"

#include "mir/options/default_configuration.h"
#include "mir/graphics/cursor.h"
#include "mir/shell/canonical_window_manager.h"

#include "mir/test/doubles/stub_display_buffer.h"
#include "mir/test/doubles/stub_renderer.h"

#include "mir/renderer/renderer_factory.h"
#include "src/server/input/null_input_manager.h"
#include "src/server/input/null_input_dispatcher.h"
#include "src/server/input/null_input_targeter.h"
#include "mir/test/doubles/null_logger.h"
#include "mir/test/doubles/stub_cursor.h"

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
class StubRendererFactory : public mir::renderer::RendererFactory
{
public:
    std::unique_ptr<mir::renderer::Renderer> create_renderer_for(mg::DisplayBuffer&)
    {
        return std::unique_ptr<mir::renderer::Renderer>(new mtd::StubRenderer());
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
                  ("tests-use-real-input", po::value<bool>()->default_value(false), "Use real input in tests.");

          return result;
      }()),
      display_rects{display_rects}
{
}

mtf::StubbedServerConfiguration::~StubbedServerConfiguration() = default;

std::shared_ptr<mg::Platform> mtf::StubbedServerConfiguration::the_graphics_platform()
{
    if (!graphics_platform)
    {
        graphics_platform = mtf::make_stubbed_server_graphics_platform(display_rects);
    }

    return graphics_platform;
}

std::shared_ptr<mir::renderer::RendererFactory> mtf::StubbedServerConfiguration::the_renderer_factory()
{
    auto options = the_options();

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

std::shared_ptr<mg::Cursor> mtf::StubbedServerConfiguration::the_cursor()
{
    return std::make_shared<mtd::StubCursor>();
}

std::shared_ptr<ml::Logger> mtf::StubbedServerConfiguration::the_logger()
{
    if (the_options()->get<bool>(mtd::logging_opt))
        return DefaultServerConfiguration::the_logger();

    return std::make_shared<mtd::NullLogger>();
}

mir::shell::WindowManagerBuilder mir_test_framework::StubbedServerConfiguration::the_window_manager_builder()
{
    return [this](msh::FocusController* focus_controller)
        { return std::make_shared<msh::CanonicalWindowManager>(
        focus_controller,
        the_shell_display_layout()); };
}
