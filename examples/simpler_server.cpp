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

#include "mir/server.h"

#include "mir/options/default_configuration.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "mir/input/composite_event_filter.h"
#include "graphics.h"

#include <linux/input.h>

#include <cstdlib>
#include <csignal>

namespace
{
class ExampleExit {};

volatile std::sig_atomic_t running = true;

void signal_handler(int /*signum*/)
{
    running = false;
}

void render_to_fb(std::shared_ptr<mir::graphics::Display> const& display)
{
    /* Set up graceful exit on SIGINT and SIGTERM */
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    mir::draw::glAnimationBasic gl_animation;

    display->for_each_display_buffer([&](mir::graphics::DisplayBuffer& buffer)
    {
        buffer.make_current();
        gl_animation.init_gl();
    });

    while (running)
    {
        display->for_each_display_buffer([&](mir::graphics::DisplayBuffer& buffer)
        {
            buffer.make_current();

            gl_animation.render_gl();

            buffer.post_update();
        });

        gl_animation.step();
    }
}

class QuitFilter : public mir::input::EventFilter
{
public:
    QuitFilter(mir::Server& server)
        : server{server}
    {
    }

    bool handle(MirEvent const& event) override
    {
        if (event.type == mir_event_type_key &&
            event.key.action == mir_key_action_down &&
            (event.key.modifiers & mir_key_modifier_alt) &&
            (event.key.modifiers & mir_key_modifier_ctrl) &&
            event.key.scan_code == KEY_BACKSPACE)
        {
            server.stop();
            return true;
        }

        return false;
    }

private:
    mir::Server& server;
};
}

int main(int argc, char const* argv[])
{
    static char const* const launch_child_opt = "launch-client";
    static char const* const render_to_fb_opt = "render-to-fb";
    mir::Server server;

    auto const quit_filter = std::make_shared<QuitFilter>(server);

    server.set_add_configuration_options(
        [] (mir::options::DefaultConfiguration& config)
        {
            namespace po = boost::program_options;

            config.add_options()
                (render_to_fb_opt, "emulate render_to_fb")
                (launch_child_opt, po::value<std::string>(), "system() command to launch client");
        });

    server.set_command_line(argc, argv);
    server.set_init_callback([&]
        {
            server.the_composite_event_filter()->append(quit_filter);

            auto const options = server.get_options();

            if (options->is_set(render_to_fb_opt))
            {
                render_to_fb(server.the_display());

                server.set_exception_handler([]{});
                throw ExampleExit{};
            }
            else if (options->is_set(launch_child_opt))
            {
                auto ignore = std::system((options->get<std::string>(launch_child_opt) + "&").c_str());
                (void)ignore;
            }
        });

    server.run();

    return server.exited_normally() ? EXIT_SUCCESS : EXIT_FAILURE;
}
