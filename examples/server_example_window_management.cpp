/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "server_example_window_management.h"
#include "server_example_shell.h"

#include "mir/server.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/graphics/display_buffer.h"
#include "mir/options/option.h"

namespace mc = mir::compositor;
namespace me = mir::examples;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace msh = mir::shell;
using namespace mir::geometry;

///\example server_example_window_management.cpp
/// Demonstrate introducing a window management strategy

namespace
{
class DisplayTracker : public mc::DisplayBufferCompositor
{
public:
    DisplayTracker(
        std::unique_ptr<mc::DisplayBufferCompositor>&& wrapped,
        Rectangle const& area,
        std::shared_ptr<me::Shell> const& window_manager) :
        wrapped{std::move(wrapped)},
        area{area},
        window_manager(window_manager)
    {
        window_manager->add_display(area);
    }

    ~DisplayTracker() noexcept
    {
        window_manager->remove_display(area);
    }

private:

    void composite(mc::SceneElementSequence&& scene_sequence) override
    {
        wrapped->composite(std::move(scene_sequence));
    }

    std::unique_ptr<mc::DisplayBufferCompositor> const wrapped;
    Rectangle const area;
    std::shared_ptr<me::Shell> const window_manager;
};

class DisplayTrackerFactory : public mc::DisplayBufferCompositorFactory
{
public:
    DisplayTrackerFactory(
        std::shared_ptr<mc::DisplayBufferCompositorFactory> const& wrapped,
        std::shared_ptr<me::Shell> const& window_manager) :
        wrapped{wrapped},
        window_manager(window_manager)
    {
    }

private:
    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(mg::DisplayBuffer& display_buffer)
    {
        auto compositor = wrapped->create_compositor_for(display_buffer);
        return std::unique_ptr<mc::DisplayBufferCompositor>{
            new DisplayTracker{std::move(compositor), display_buffer.view_area(), window_manager}};
    }

    std::shared_ptr<mc::DisplayBufferCompositorFactory> const wrapped;
    std::shared_ptr<me::Shell> const window_manager;
};
}

void me::add_window_manager_option_to(Server& server)
{
    server.add_configuration_option(me::wm_option, me::wm_description, mir::OptionType::string);

    auto const factory = std::make_shared<me::ShellFactory>(server);

    server.override_the_shell([factory, &server]()
        -> std::shared_ptr<msh::Shell>
        {
            auto const options = server.get_options();

            if (!options->is_set(me::wm_option))
                return std::shared_ptr<msh::Shell>{};

            return factory->shell();
        });

    server.wrap_display_buffer_compositor_factory([factory, &server]
       (std::shared_ptr<mc::DisplayBufferCompositorFactory> const& wrapped)
       -> std::shared_ptr<mc::DisplayBufferCompositorFactory>
       {
           auto const options = server.get_options();

           if (!options->is_set(me::wm_option))
               return wrapped;

           return std::make_shared<DisplayTrackerFactory>(wrapped, factory->shell());
       });
}
