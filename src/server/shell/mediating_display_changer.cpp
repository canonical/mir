/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/shell/mediating_display_changer.h"
#include "mir/shell/session_container.h"
#include "mir/shell/session.h"
#include "mir/graphics/display.h"
#include "mir/compositor/compositor.h"
#include "mir/input/input_manager.h"
#include "mir/graphics/display_configuration_policy.h"

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mi = mir::input;

namespace
{

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

msh::MediatingDisplayChanger::MediatingDisplayChanger(
    std::shared_ptr<mg::Display> const& display,
    std::shared_ptr<mc::Compositor> const& compositor,
    std::shared_ptr<mi::InputManager> const& input_manager,
    std::shared_ptr<mg::DisplayConfigurationPolicy> const& display_configuration_policy,
    std::shared_ptr<msh::SessionContainer> const& session_container)
    : display{display},
      compositor{compositor},
      input_manager{input_manager},
      display_configuration_policy{display_configuration_policy},
      session_container{session_container}
{
}

void msh::MediatingDisplayChanger::configure(
    std::weak_ptr<mf::Session> const& session,
    std::shared_ptr<mg::DisplayConfiguration> const& conf)
{
    apply_config(conf, PauseResumeSystem);
    send_config_to_all_sessions_except(conf, session);
}

std::shared_ptr<mg::DisplayConfiguration>
msh::MediatingDisplayChanger::active_configuration()
{
    std::lock_guard<std::mutex> lg{configuration_mutex};
    return display->configuration();
}

void msh::MediatingDisplayChanger::configure_for_hardware_change(
    std::shared_ptr<graphics::DisplayConfiguration> const& conf,
    SystemStateHandling pause_resume_system)
{
    display_configuration_policy->apply_to(*conf);
    apply_config(conf, pause_resume_system);
    send_config_to_all_sessions_except(conf, {});
}

void msh::MediatingDisplayChanger::apply_config(
    std::shared_ptr<graphics::DisplayConfiguration> const& conf,
    SystemStateHandling pause_resume_system)
{
    std::lock_guard<std::mutex> lg{configuration_mutex};

    if (pause_resume_system)
    {
        ApplyNowAndRevertOnScopeExit comp{
            [this] { compositor->stop(); },
            [this] { compositor->start(); }};

        ApplyNowAndRevertOnScopeExit input{
            [this] { input_manager->stop(); },
            [this] { input_manager->start(); }};

        display->configure(*conf);
    }
    else
    {
        display->configure(*conf);
    }
}

void msh::MediatingDisplayChanger::send_config_to_all_sessions_except(
    std::shared_ptr<mg::DisplayConfiguration> const& conf,
    std::weak_ptr<mf::Session> const& excluded_session)
{
    auto excluded = excluded_session.lock();

    session_container->for_each(
        [&conf, &excluded](std::shared_ptr<msh::Session> const& session)
        {
            if (session != excluded)
                session->send_display_config(*conf);
        });
}
