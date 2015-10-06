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

#include <condition_variable>
#include "mediating_display_changer.h"
#include "session_container.h"
#include "mir/scene/session.h"
#include "session_event_handler_register.h"
#include "mir/graphics/display.h"
#include "mir/compositor/compositor.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/display_configuration_report.h"
#include "mir/server_action_queue.h"

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
}

ms::MediatingDisplayChanger::MediatingDisplayChanger(
    std::shared_ptr<mg::Display> const& display,
    std::shared_ptr<mc::Compositor> const& compositor,
    std::shared_ptr<mg::DisplayConfigurationPolicy> const& display_configuration_policy,
    std::shared_ptr<SessionContainer> const& session_container,
    std::shared_ptr<SessionEventHandlerRegister> const& session_event_handler_register,
    std::shared_ptr<ServerActionQueue> const& server_action_queue,
    std::shared_ptr<mg::DisplayConfigurationReport> const& report)
    : display{display},
      compositor{compositor},
      display_configuration_policy{display_configuration_policy},
      session_container{session_container},
      session_event_handler_register{session_event_handler_register},
      server_action_queue{server_action_queue},
      report{report},
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

    report->initial_configuration(*base_configuration);
}

void ms::MediatingDisplayChanger::configure(
    std::shared_ptr<mf::Session> const& session,
    std::shared_ptr<mg::DisplayConfiguration> const& conf)
{
    bool is_active_session{false};
    {
        std::lock_guard<std::mutex> lg{configuration_mutex};
        config_map[session] = conf;
        is_active_session = session == focused_session.lock();
    }

    if (is_active_session)
    {
        std::weak_ptr<mf::Session> const weak_session{session};
        std::condition_variable cv;
        bool done{false};

        server_action_queue->enqueue(
            this,
            [this, weak_session, conf, &done, &cv]
            {
                std::lock_guard<std::mutex> lg{configuration_mutex};

                if (auto const session = weak_session.lock())
                {
                    /* If the session is focused, apply the configuration */
                    if (focused_session.lock() == session)
                        apply_config(conf, PauseResumeSystem);
                }

                done = true;
                cv.notify_one();
            });

        std::unique_lock<std::mutex> lg{configuration_mutex};
        cv.wait(lg, [&done] { return done; });
    }
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
    report->new_configuration(*conf);
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
        session->send_display_config(*it->second);
    }
    else if (!base_configuration_applied)
    {
        apply_base_config(PauseResumeSystem);
        session->send_display_config(*base_configuration);
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

std::future<void> ms::MediatingDisplayChanger::set_default_display_configuration(
    std::shared_ptr<mg::DisplayConfiguration> const &conf)
{
    auto promise = std::make_shared<std::promise<void>>();
    auto completion_future = promise->get_future();
    server_action_queue->enqueue(
        this,
        [this, conf, done = std::move(promise)]
        {
            std::lock_guard<std::mutex> lg{configuration_mutex};

            base_configuration = conf;
            if (base_configuration_applied)
            {
                apply_base_config(PauseResumeSystem);

                /* Send the new configuration to all the sessions */
                send_config_to_all_sessions(conf);
            }
            done->set_value();
        });
    return completion_future;
}
