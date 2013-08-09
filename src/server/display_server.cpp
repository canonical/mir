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

#include "mir/compositor/compositor.h"
#include "mir/frontend/communicator.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_configuration_policy.h"
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

class ApplyNowAndRevertOnScopeExit
{
public:
    ApplyNowAndRevertOnScopeExit(std::function<void()> const& apply,
                                 std::function<void()> const& revert)
        : revert{revert}
    {
        apply();
    }

    ~ApplyNowAndRevertOnScopeExit()
    {
        revert();
    }

private:
    ApplyNowAndRevertOnScopeExit(ApplyNowAndRevertOnScopeExit const&) = delete;
    ApplyNowAndRevertOnScopeExit& operator=(ApplyNowAndRevertOnScopeExit const&) = delete;

    std::function<void()> const revert;
};

}

struct mir::DisplayServer::Private
{
    Private(ServerConfiguration& config)
        : display{config.the_display()},
          compositor{config.the_compositor()},
          communicator{config.the_communicator()},
          input_manager{config.the_input_manager()},
          main_loop{config.the_main_loop()},
          display_configuration_policy{config.the_display_configuration_policy()},
          paused{false},
          configure_display_on_resume{false}
    {
        display->register_configuration_change_handler(
            *main_loop,
            [this] { return configure_display_handler(); });

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
                [this] { communicator->stop(); },
                [this] { communicator->start(); }};

            display->pause();

            paused = true;
        }
        catch(std::runtime_error const&)
        {
            return false;
        }

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
                [this] { communicator->start(); },
                [this] { communicator->stop(); }};

            if (configure_display_on_resume)
            {
                configure_display();
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

        return true;
    }

    void configure_display()
    {
        auto conf = display->configuration();
        display_configuration_policy->apply_to(*conf);
        display->configure(*conf);
    }

    void configure_display_handler()
    {
        if (!paused)
        {
            ApplyNowAndRevertOnScopeExit comp{
                [this] { compositor->stop(); },
                [this] { compositor->start(); }};

            ApplyNowAndRevertOnScopeExit input{
                [this] { input_manager->stop(); },
                [this] { input_manager->start(); }};

            configure_display();
        }
        else
        {
            configure_display_on_resume = true;
        }
    }

    std::shared_ptr<mg::Display> display;
    std::shared_ptr<mc::Compositor> compositor;
    std::shared_ptr<mf::Communicator> communicator;
    std::shared_ptr<mi::InputManager> input_manager;
    std::shared_ptr<mir::MainLoop> main_loop;
    std::shared_ptr<mg::DisplayConfigurationPolicy> const display_configuration_policy;
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
    p->communicator->start();
    p->compositor->start();
    p->input_manager->start();

    p->main_loop->run();

    p->input_manager->stop();
    p->compositor->stop();
    p->communicator->stop();
}

void mir::DisplayServer::stop()
{
    p->main_loop->stop();
}
