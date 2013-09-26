/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/display_server.h"
#include "mir/server_configuration.h"
#include "mir/main_loop.h"
#include "mir/pause_resume_listener.h"
#include "mir/display_changer.h"

#include "mir/compositor/compositor.h"
#include "mir/frontend/connector.h"
#include "mir/graphics/display.h"
#include "mir/input/input_manager.h"

#include <stdexcept>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace msh = mir::shell;

namespace
{

class TryButRevertIfUnwinding
{
public:
    TryButRevertIfUnwinding(std::function<void()> const& apply,
                            std::function<void()> const& revert)
        : revert{revert}
    {
        apply();
    }

    ~TryButRevertIfUnwinding()
    {
        if (std::uncaught_exception())
            revert();
    }

private:
    TryButRevertIfUnwinding(TryButRevertIfUnwinding const&) = delete;
    TryButRevertIfUnwinding& operator=(TryButRevertIfUnwinding const&) = delete;

    std::function<void()> const revert;
};

}

struct mir::DisplayServer::Private
{
    Private(ServerConfiguration& config)
        : graphics_platform{config.the_graphics_platform()},
          display{config.the_display()},
          input_configuration{config.the_input_configuration()},
          compositor{config.the_compositor()},
          connector{config.the_connector()},
          input_manager{config.the_input_manager()},
          main_loop{config.the_main_loop()},
          pause_resume_listener{config.the_pause_resume_listener()},
          display_changer{config.the_display_changer()},
          paused{false},
          configure_display_on_resume{false}
    {
        display->register_configuration_change_handler(
            *main_loop,
            [this] { return configure_display(); });

        display->register_pause_resume_handlers(
            *main_loop,
            [this] { return pause(); },
            [this] { return resume(); });
    }

    bool pause()
    {
        try
        {
            TryButRevertIfUnwinding input{
                [this] { input_manager->stop(); },
                [this] { input_manager->start(); }};

            TryButRevertIfUnwinding comp{
                [this] { compositor->stop(); },
                [this] { compositor->start(); }};

            TryButRevertIfUnwinding comm{
                [this] { connector->stop(); },
                [this] { connector->start(); }};

            display->pause();

            paused = true;
        }
        catch(std::runtime_error const&)
        {
            return false;
        }

        pause_resume_listener->paused();

        return true;
    }

    bool resume()
    {
        try
        {
            TryButRevertIfUnwinding disp{
                [this] { display->resume(); },
                [this] { display->pause(); }};

            TryButRevertIfUnwinding comm{
                [this] { connector->start(); },
                [this] { connector->stop(); }};

            if (configure_display_on_resume)
            {
                auto conf = display->configuration();
                display_changer->configure_for_hardware_change(
                    conf, DisplayChanger::RetainSystemState);
                configure_display_on_resume = false;
            }

            TryButRevertIfUnwinding input{
                [this] { input_manager->start(); },
                [this] { input_manager->stop(); }};

            compositor->start();

            paused = false;
        }
        catch(std::runtime_error const&)
        {
            return false;
        }

        pause_resume_listener->resumed();

        return true;
    }

    void configure_display()
    {
        if (!paused)
        {
            auto conf = display->configuration();
            display_changer->configure_for_hardware_change(
                conf, DisplayChanger::PauseResumeSystem);
        }
        else
        {
            configure_display_on_resume = true;
        }
    }

    std::shared_ptr<mg::Platform> const graphics_platform; // Hold this so the platform is loaded once
    std::shared_ptr<mg::Display> const display;
    std::shared_ptr<input::InputConfiguration> const input_configuration;
    std::shared_ptr<mc::Compositor> const compositor;
    std::shared_ptr<mf::Connector> const connector;
    std::shared_ptr<mi::InputManager> const input_manager;
    std::shared_ptr<mir::MainLoop> const main_loop;
    std::shared_ptr<mir::PauseResumeListener> const pause_resume_listener;
    std::shared_ptr<mir::DisplayChanger> const display_changer;
    bool paused;
    bool configure_display_on_resume;
};

mir::DisplayServer::DisplayServer(ServerConfiguration& config) :
    p(new DisplayServer::Private{config})
{
}

/*
 * Need to define the destructor in the source file, so that we
 * can define the 'p' member variable as a unique_ptr to an
 * incomplete type (DisplayServerPrivate) in the header.
 */
mir::DisplayServer::~DisplayServer()
{
}

void mir::DisplayServer::run()
{
    p->connector->start();
    p->compositor->start();
    p->input_manager->start();

    p->main_loop->run();

    p->input_manager->stop();
    p->compositor->stop();
    p->connector->stop();
}

void mir::DisplayServer::stop()
{
    p->main_loop->stop();
}
