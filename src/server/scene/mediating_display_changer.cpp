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

#include "mediating_display_changer.h"
#include "session_container.h"
#include "mir/scene/session.h"
#include "session_event_handler_register.h"
#include "mir/graphics/display.h"
#include "mir/compositor/compositor.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/display_configuration.h"
#include "mir/server_action_queue.h"
#include "mir/log.h"
#include <cmath>

namespace mf = mir::frontend;
namespace ms = mir::scene;
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

void log_configuration(mg::DisplayConfiguration const& conf)
{
    conf.for_each_output([](mg::DisplayConfigurationOutput const& out)
    {
        static const char* const type_str[] =
            {"Unknown", "VGA", "DVI-I", "DVI-D", "DVI-A", "Composite",
             "S-Video", "LVDS", "Component", "9-pin-DIN", "DisplayPort",
             "HDMI-A", "HDMI-B", "TV", "eDP"};
        auto type = type_str[static_cast<int>(out.type)];
        int out_id = out.id.as_value();
        int card_id = out.card_id.as_value();
        const char prefix[] = "  ";

        if (out.connected)
        {
            int width_mm = out.physical_size_mm.width.as_int();
            int height_mm = out.physical_size_mm.height.as_int();
            float inches =
                sqrtf(width_mm * width_mm + height_mm * height_mm) / 25.4;
            int indent = 0;
            mir::log_info("%s%d.%d: %n%s %.1f\" %dx%dmm",
                          prefix, card_id, out_id, &indent, type,
                          inches, width_mm, height_mm);
            if (out.used)
            {
                if (out.current_mode_index < out.modes.size())
                {
                    auto const& mode = out.modes[out.current_mode_index];
                    mir::log_info("%*cCurrent mode %dx%d %.2fHz",
                                  indent, ' ',
                                  mode.size.width.as_int(),
                                  mode.size.height.as_int(),
                                  mode.vrefresh_hz);
                }
                if (out.preferred_mode_index < out.modes.size())
                {
                    auto const& mode = out.modes[out.preferred_mode_index];
                    mir::log_info("%*cPreferred mode %dx%d %.2fHz",
                                  indent, ' ',
                                  mode.size.width.as_int(),
                                  mode.size.height.as_int(),
                                  mode.vrefresh_hz);
                }
                mir::log_info("%*cLogical position %+d%+d",
                              indent, ' ',
                              out.top_left.x.as_int(),
                              out.top_left.y.as_int());
            }
            else
            {
                mir::log_info("%*cDisabled", indent, ' ');
            }
        }
        else
        {
            mir::log_info("%s%d.%d: unused %s", prefix, card_id, out_id, type);
        }
    });
}

}

ms::MediatingDisplayChanger::MediatingDisplayChanger(
    std::shared_ptr<mg::Display> const& display,
    std::shared_ptr<mc::Compositor> const& compositor,
    std::shared_ptr<mg::DisplayConfigurationPolicy> const& display_configuration_policy,
    std::shared_ptr<SessionContainer> const& session_container,
    std::shared_ptr<SessionEventHandlerRegister> const& session_event_handler_register,
    std::shared_ptr<ServerActionQueue> const& server_action_queue)
    : display{display},
      compositor{compositor},
      display_configuration_policy{display_configuration_policy},
      session_container{session_container},
      session_event_handler_register{session_event_handler_register},
      server_action_queue{server_action_queue},
      base_configuration{display->configuration()},
      base_configuration_applied{true}
{
    session_event_handler_register->register_focus_change_handler(
        [this](std::shared_ptr<ms::Session> const& session)
        {
            auto const weak_session = std::weak_ptr<ms::Session>(session);
            this->server_action_queue->enqueue(
                this,
                [this,weak_session]
                {
                    if (auto const session = weak_session.lock())
                        focus_change_handler(session);
                });
        });

    session_event_handler_register->register_no_focus_handler(
        [this]
        {
            this->server_action_queue->enqueue(
                this,
                [this] { no_focus_handler(); });
        });

    session_event_handler_register->register_session_stopping_handler(
        [this](std::shared_ptr<ms::Session> const& session)
        {
            auto const weak_session = std::weak_ptr<ms::Session>(session);
            this->server_action_queue->enqueue(
                this,
                [this,weak_session]
                {
                    if (auto const session = weak_session.lock())
                        session_stopping_handler(session);
                });
        });
    mir::log_info("Initial display configuration:");
    log_configuration(*base_configuration);
}

void ms::MediatingDisplayChanger::configure(
    std::shared_ptr<mf::Session> const& session,
    std::shared_ptr<mg::DisplayConfiguration> const& conf)
{
    server_action_queue->enqueue(
        this,
        [this, session, conf]
        {
            std::lock_guard<std::mutex> lg{configuration_mutex};

            config_map[session] = conf;

            /* If the session is focused, apply the configuration */
            if (focused_session.lock() == session)
            {
                apply_config(conf, PauseResumeSystem);
            }
        });
}

std::shared_ptr<mg::DisplayConfiguration>
ms::MediatingDisplayChanger::active_configuration()
{
    std::lock_guard<std::mutex> lg{configuration_mutex};

    return display->configuration();
}

void ms::MediatingDisplayChanger::configure_for_hardware_change(
    std::shared_ptr<graphics::DisplayConfiguration> const& conf,
    SystemStateHandling pause_resume_system)
{
    server_action_queue->enqueue(
        this,
        [this, conf, pause_resume_system]
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
        });
}

void ms::MediatingDisplayChanger::pause_display_config_processing()
{
    server_action_queue->pause_processing_for(this);
}

void ms::MediatingDisplayChanger::resume_display_config_processing()
{
    server_action_queue->resume_processing_for(this);
}

void ms::MediatingDisplayChanger::apply_config(
    std::shared_ptr<graphics::DisplayConfiguration> const& conf,
    SystemStateHandling pause_resume_system)
{
    mir::log_info("New display configuration:");
    log_configuration(*conf);
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

void ms::MediatingDisplayChanger::apply_base_config(
    SystemStateHandling pause_resume_system)
{
    apply_config(base_configuration, pause_resume_system);
    base_configuration_applied = true;
}

void ms::MediatingDisplayChanger::send_config_to_all_sessions(
    std::shared_ptr<mg::DisplayConfiguration> const& conf)
{
    session_container->for_each(
        [&conf](std::shared_ptr<Session> const& session)
        {
            session->send_display_config(*conf);
        });
}

void ms::MediatingDisplayChanger::focus_change_handler(
    std::shared_ptr<ms::Session> const& session)
{
    std::lock_guard<std::mutex> lg{configuration_mutex};

    focused_session = session;

    /*
     * If the newly focused session has a display configuration, apply it.
     * Otherwise if we aren't currently using the base configuration,
     * apply that.
     */
    auto const it = config_map.find(session);
    if (it != config_map.end())
    {
        apply_config(it->second, PauseResumeSystem);
    }
    else if (!base_configuration_applied)
    {
        apply_base_config(PauseResumeSystem);
    }
}

void ms::MediatingDisplayChanger::no_focus_handler()
{
    std::lock_guard<std::mutex> lg{configuration_mutex};

    focused_session.reset();
    if (!base_configuration_applied)
    {
        apply_base_config(PauseResumeSystem);
    }
}

void ms::MediatingDisplayChanger::session_stopping_handler(
    std::shared_ptr<ms::Session> const& session)
{
    std::lock_guard<std::mutex> lg{configuration_mutex};

    config_map.erase(session);
}
