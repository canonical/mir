/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/command_line_server_configuration.h"

#include "mir_test_framework/stub_server_platform_factory.h"

#include "mir/options/default_configuration.h"
#include "mir/graphics/cursor.h"

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
    auto create_renderer_for(
        std::unique_ptr<mg::gl::OutputSurface>,
        std::shared_ptr<mg::GLRenderingProvider> /*platform*/)
        const -> std::unique_ptr<mir::renderer::Renderer> override
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

auto mtf::StubbedServerConfiguration::the_display_platforms()
    -> std::vector<std::shared_ptr<graphics::DisplayPlatform>> const&
{
    if (display_platform.empty())
    {
        display_platform.push_back(mtf::make_stubbed_display_platform(display_rects));
    }
    return display_platform;
}

auto mtf::StubbedServerConfiguration::the_rendering_platforms()
    -> std::vector<std::shared_ptr<graphics::RenderingPlatform>> const&
{
    if (rendering_platform.empty())
    {
        rendering_platform.push_back(mtf::make_stubbed_rendering_platform());
    }
    return rendering_platform;
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
