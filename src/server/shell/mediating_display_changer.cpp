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
#include "mir/shell/session_event_handler_register.h"
#include "mir/graphics/display.h"
#include "mir/compositor/compositor.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/display_configuration.h"

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mc = mir::compositor;

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
    std::shared_ptr<mg::DisplayConfigurationPolicy> const& display_configuration_policy,
    std::shared_ptr<msh::SessionContainer> const& session_container,
    std::shared_ptr<SessionEventHandlerRegister> const& session_event_handler_register)
    : display{display},
      compositor{compositor},
      display_configuration_policy{display_configuration_policy},
      session_container{session_container},
      session_event_handler_register{session_event_handler_register},
      base_configuration{display->configuration()},
      base_configuration_applied{true}
{
    session_event_handler_register->register_focus_change_handler(
        [this](std::shared_ptr<Session> const& session)
        {
            std::lock_guard<std::mutex> lg{configuration_mutex};

            focused_session = session;

            /*
             * If the newly focused session has a display configuration, apply it.
             * Otherwise if we aren't currently using the base configuration,
             * apply that.
             */
            auto it = config_map.find(session);
            if (it != config_map.end())
            {
                apply_config(it->second, PauseResumeSystem);
            }
            else if (!base_configuration_applied)
            {
                apply_base_config(PauseResumeSystem);
            }
        });

    session_event_handler_register->register_no_focus_handler(
        [this]
        {
            std::lock_guard<std::mutex> lg{configuration_mutex};

            focused_session.reset();
            if (!base_configuration_applied)
            {
                apply_base_config(PauseResumeSystem);
            }
        });

    session_event_handler_register->register_session_stopping_handler(
        [this](std::shared_ptr<Session> const& session)
        {
            std::lock_guard<std::mutex> lg{configuration_mutex};

            config_map.erase(session);
        });

}

void msh::MediatingDisplayChanger::ensure_display_powered(std::shared_ptr<mf::Session> const& session)
{
    std::lock_guard<std::mutex> lg{configuration_mutex};
    bool switched = false;

    auto it = config_map.find(session);
    if (it == config_map.end())
        return;
    auto conf = it->second;
    conf->for_each_output([&](mg::DisplayConfigurationOutput const& output) -> void
    {
        if (!output.used) return;
        
        if (output.power_mode != mir_power_mode_on)
        {
            switched = true;
            conf->configure_output(output.id, output.used,
                                   output.top_left, 
                                   output.current_mode_index,
                                   mir_power_mode_on);
        }
    });
    if (switched)
        configure(session, conf);
}

void msh::MediatingDisplayChanger::configure(
    std::shared_ptr<mf::Session> const& session,
    std::shared_ptr<mg::DisplayConfiguration> const& conf)
{
    std::lock_guard<std::mutex> lg{configuration_mutex};

    config_map[session] = conf;

    /* If the session is focused, apply the configuration */
    if (focused_session.lock() == session)
    {
        apply_config(conf, PauseResumeSystem);
    }
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
    std::lock_guard<std::mutex> lg{configuration_mutex};

    display_configuration_policy->apply_to(*conf);
    base_configuration = conf;
    if (base_configuration_applied)
        apply_base_config(pause_resume_system);

    /*
     * Clear all the per-session configurations, since they may have become
     * invalid due to the hardware change.
     */
    config_map.clear();

    /* Send the new configuration to all the sessions */
    send_config_to_all_sessions(conf);
}

void msh::MediatingDisplayChanger::apply_config(
    std::shared_ptr<graphics::DisplayConfiguration> const& conf,
    SystemStateHandling pause_resume_system)
{
    if (pause_resume_system)
    {
        ApplyNowAndRevertOnScopeExit comp{
            [this] { compositor->stop(); },
            [this] { compositor->start(); }};

        display->configure(*conf);
    }
    else
    {
        display->configure(*conf);
    }

    base_configuration_applied = false;
}

void msh::MediatingDisplayChanger::apply_base_config(
    SystemStateHandling pause_resume_system)
{
    apply_config(base_configuration, pause_resume_system);
    base_configuration_applied = true;
}

void msh::MediatingDisplayChanger::send_config_to_all_sessions(
    std::shared_ptr<mg::DisplayConfiguration> const& conf)
{
    session_container->for_each(
        [&conf](std::shared_ptr<msh::Session> const& session)
        {
            session->send_display_config(*conf);
        });
}
