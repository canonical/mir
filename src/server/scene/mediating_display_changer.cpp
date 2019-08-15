/*
 * Copyright © 2013-2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include <condition_variable>
#include <boost/throw_exception.hpp>
#include <unordered_set>
#include "mediating_display_changer.h"
#include "mir/scene/session_container.h"
#include "mir/scene/session.h"
#include "mir/scene/session_event_handler_register.h"
#include "mir/scene/session_event_sink.h"
#include "mir/graphics/display.h"
#include "mir/compositor/compositor.h"
#include "mir/geometry/rectangles.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/display_configuration_observer.h"
#include "mir/server_action_queue.h"
#include "mir/time/alarm_factory.h"
#include "mir/time/alarm.h"
#include "mir/client_visible_error.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mt = mir::time;

namespace
{
char const* const bad_display_config = "Invalid or inconsistent display configuration";

class DisplayConfigurationInProgressError : public mir::ClientVisibleError
{
public:
    DisplayConfigurationInProgressError()
        : ClientVisibleError("Base display configuration already in progress")
    {
    }

    MirErrorDomain domain() const noexcept override
    {
        return mir_error_domain_display_configuration;
    }

    uint32_t code() const noexcept override
    {
        return mir_display_configuration_error_in_progress;
    }
};

class DisplayConfigurationNotInProgressError : public mir::ClientVisibleError
{
public:
    DisplayConfigurationNotInProgressError()
        : ClientVisibleError("No base display configuration preview in progress")
    {
    }

    MirErrorDomain domain() const noexcept override
    {
        return mir_error_domain_display_configuration;
    }

    uint32_t code() const noexcept override
    {
        return mir_display_configuration_error_no_preview_in_progress;
    }
};

class DisplayConfigurationFailedError : public mir::ClientVisibleError
{
public:
    DisplayConfigurationFailedError()
        : ClientVisibleError("Display configuration falied")
    {
    }

    MirErrorDomain domain() const noexcept override
    {
        return mir_error_domain_display_configuration;
    }

    uint32_t code() const noexcept override
    {
        return mir_display_configuration_error_rejected_by_hardware;
    }
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

struct ms::MediatingDisplayChanger::SessionObserver : ms::SessionEventSink
{
    SessionObserver(ms::MediatingDisplayChanger* self) : self{self} {}

    void handle_focus_change(std::shared_ptr<mir::scene::Session> const& session) override
    {
        auto const weak_session = std::weak_ptr<ms::Session>(session);
        self->server_action_queue->enqueue(
            self,
            [self=self,weak_session]
                {
                    if (auto const session = weak_session.lock())
                        self->focus_change_handler(session);
                });
    }

    void handle_no_focus() override
    {
        self->server_action_queue->enqueue(
            self,
            [self=self] { self->no_focus_handler(); });
    }

    void handle_session_stopping(std::shared_ptr<mir::scene::Session> const& session) override
    {
        auto const weak_session = std::weak_ptr<ms::Session>(session);
        self->server_action_queue->enqueue(
            self,
            [self=self,weak_session]
            {
                if (auto const session = weak_session.lock())
                    self->session_stopping_handler(session);
            });
    }

    ms::MediatingDisplayChanger* const self;
};


ms::MediatingDisplayChanger::MediatingDisplayChanger(
    std::shared_ptr<mg::Display> const& display,
    std::shared_ptr<mc::Compositor> const& compositor,
    std::shared_ptr<mg::DisplayConfigurationPolicy> const& display_configuration_policy,
    std::shared_ptr<SessionContainer> const& session_container,
    std::shared_ptr<SessionEventHandlerRegister> const& session_event_handler_register,
    std::shared_ptr<ServerActionQueue> const& server_action_queue,
    std::shared_ptr<mg::DisplayConfigurationObserver> const& observer,
    std::shared_ptr<mt::AlarmFactory> const& alarm_factory)
    : display{display},
      compositor{compositor},
      display_configuration_policy{display_configuration_policy},
      session_container{session_container},
      session_event_handler_register{session_event_handler_register},
      server_action_queue{server_action_queue},
      observer{observer},
      base_configuration_{display->configuration()},
      base_configuration_applied{true},
      alarm_factory{alarm_factory},
      session_observer{std::make_unique<SessionObserver>(this)}
{
    session_event_handler_register->add(session_observer.get());
    observer->initial_configuration(base_configuration_);
}

ms::MediatingDisplayChanger::~MediatingDisplayChanger()
{
    session_event_handler_register->remove(session_observer.get());
}

void ms::MediatingDisplayChanger::configure(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<mg::DisplayConfiguration> const& conf)
{
    if (!conf->valid())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(bad_display_config));
    }

    {
        std::lock_guard<std::mutex> lg{configuration_mutex};
        config_map[session] = conf;
        observer->session_configuration_applied(session, conf);

        if (session != focused_session.lock())
            return;
    }

    std::weak_ptr<ms::Session> const weak_session{session};

    server_action_queue->enqueue(
        this,
        [this, weak_session, conf]
        {
            if (auto const session = weak_session.lock())
            {
                std::lock_guard<std::mutex> lg{configuration_mutex};

                /* If the session is focused, apply the configuration */
                if (focused_session.lock() == session)
                {
                    try
                    {
                        apply_config(conf);
                    }
                    catch (std::exception const&)
                    {
                        session->send_error(DisplayConfigurationFailedError{});
                    }
                }
            }
        });
}

void ms::MediatingDisplayChanger::remove_session_configuration(
    std::shared_ptr<ms::Session> const& session)
{
    {
        std::lock_guard<std::mutex> lg{configuration_mutex};
        if (config_map.find(session) != config_map.end())
        {
            config_map.erase(session);
            observer->session_configuration_removed(session);
        }

        if (session != focused_session.lock())
            return;
    }

    std::weak_ptr<ms::Session> const weak_session{session};

    server_action_queue->enqueue(
        this,
        [this, weak_session]
        {
            if (auto const session = weak_session.lock())
            {
                std::lock_guard<std::mutex> lg{configuration_mutex};

                try
                {
                    apply_base_config();
                }
                catch (std::exception const&)
                {
                    session->send_error(DisplayConfigurationFailedError{});
                }
            }
        });
}

void
ms::MediatingDisplayChanger::preview_base_configuration(
    std::weak_ptr<ms::Session> const& session,
    std::shared_ptr<graphics::DisplayConfiguration> const& conf,
    std::chrono::seconds timeout)
{
    if (!conf->valid())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(bad_display_config));
    }

    {
        std::lock_guard<std::mutex> lock{configuration_mutex};

        if (preview_configuration_timeout)
        {
            BOOST_THROW_EXCEPTION(
                DisplayConfigurationInProgressError());
        }

        preview_configuration_timeout = alarm_factory->create_alarm(
            [this, session]()
                {
                    if (auto live_session = session.lock())
                    {
                        apply_base_config();
                        live_session->send_display_config(*base_configuration());
                    }
                });
        preview_configuration_timeout->reschedule_in(timeout);
        currently_previewing_session = session;
    }

    server_action_queue->enqueue(
        this,
        [this, conf, session]()
        {
            if (auto live_session = session.lock())
            {
                try
                {
                    apply_config(conf);
                    live_session->send_display_config(*conf);
                }
                catch (std::runtime_error const&)
                {
                    live_session->send_error(DisplayConfigurationFailedError{});
                }
            }
        });
}

void
ms::MediatingDisplayChanger::confirm_base_configuration(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<graphics::DisplayConfiguration> const& confirmed_conf)
{
    {
        std::lock_guard<std::mutex> lock{configuration_mutex};
        preview_configuration_timeout = std::unique_ptr<mt::Alarm>();
        currently_previewing_session = std::weak_ptr<ms::Session>{};
    }
    set_base_configuration(confirmed_conf);
}

void
ms::MediatingDisplayChanger::cancel_base_configuration_preview(
    std::shared_ptr<ms::Session> const& session)
{
    std::unique_ptr<mt::Alarm> previously_set_alarm;
    {
        std::lock_guard<std::mutex> lock{configuration_mutex};
        if (!preview_configuration_timeout || (currently_previewing_session.lock() != session))
        {
            BOOST_THROW_EXCEPTION(DisplayConfigurationNotInProgressError());
        }
        /* We transfer the alarm out of the lock, because the alarm's callback may try
         * to *take* the configuration_mutex lock.
         *
         * Since cancel() blocks until the callback will definitely not be entered,
         * we need to call cancel() outside the configuration_mutex lock to avoid possible
         * deadlocks
         */
        previously_set_alarm = std::move(preview_configuration_timeout);
        preview_configuration_timeout = nullptr;
        currently_previewing_session = std::weak_ptr<ms::Session>{};
    }

    if (previously_set_alarm->cancel())
    {
        // We cancelled the alarm, which means it had not already been triggered.
        // Therefore we need to queue up a switch back to the base display configuration and
        // send a notification.
        server_action_queue->enqueue(
            this,
            [this, weak_session = std::weak_ptr<ms::Session>(session)]()
            {
                if (auto live_session = weak_session.lock())
                {
                    apply_base_config();
                    live_session->send_display_config(*base_configuration());
                }
            });
    }
}

std::shared_ptr<mg::DisplayConfiguration>
ms::MediatingDisplayChanger::base_configuration()
{
    std::lock_guard<std::mutex> lg{configuration_mutex};

    return base_configuration_->clone();
}

void ms::MediatingDisplayChanger::configure_for_hardware_change(
    std::shared_ptr<graphics::DisplayConfiguration> const& conf)
{
    server_action_queue->enqueue(
        this,
        [this, conf]
        {
            std::lock_guard<std::mutex> lg{configuration_mutex};

            auto existing_configuration = base_configuration_;

            display_configuration_policy->apply_to(*conf);
            base_configuration_ = conf;
            if (base_configuration_applied)
            {
                try
                {
                    apply_base_config();
                    observer->base_configuration_updated(base_configuration_);
                }
                catch (std::exception const&)
                {
                    // TODO: Notify someone.
                    base_configuration_ = existing_configuration;
                }
            }

            /*
             * Clear all the per-session configurations, since they may have become
             * invalid due to the hardware change.
             */
            config_map.clear();

            /* Send the new configuration to all the sessions */
            send_config_to_all_sessions(base_configuration_);
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

namespace
{
bool configuration_has_new_outputs_enabled(
    mg::DisplayConfiguration const& existing,
    mg::DisplayConfiguration const& updated)
{
    std::unordered_set<mg::DisplayConfigurationOutputId> currently_enabled_outputs;
    existing.for_each_output(
        [&currently_enabled_outputs](auto const& output)
        {
            if (output.used)
            {
                currently_enabled_outputs.insert(output.id);
            }
        });
    bool has_new_output{false};
    updated.for_each_output(
        [&currently_enabled_outputs, &has_new_output](auto const& output)
        {
            if (output.used)
            {
                has_new_output |= (currently_enabled_outputs.count(output.id) == 0);
            }
        });
    return has_new_output;
}
}

void ms::MediatingDisplayChanger::apply_config(
    std::shared_ptr<graphics::DisplayConfiguration> const& conf)
{
    auto existing_configuration = display->configuration();
    try
    {
        if (configuration_has_new_outputs_enabled(*display->configuration(), *conf) ||
            !display->apply_if_configuration_preserves_display_buffers(*conf))
        {
            ApplyNowAndRevertOnScopeExit comp{
                [this] { compositor->stop(); },
                [this] { compositor->start(); }};
            display->configure(*conf);
        }

        observer->configuration_applied(conf);
        base_configuration_applied = false;
    }
    catch (std::exception const& e)
    {
        try
        {
            /*
             * Reapply the previous configuration.
             *
             * We know that the previous configuration worked at some point - either it
             * was one that has been successfully display->configure()d, or it was the
             * configuration that existed at Mir startup. Which presumably worked!
             */
            ApplyNowAndRevertOnScopeExit comp{
                [this] { compositor->stop(); },
                [this] { compositor->start(); }};
            display->configure(*existing_configuration);
        }
        catch (std::exception const& e)
        {
            observer->catastrophic_configuration_error(std::move(existing_configuration), e);
            throw;
        }
        observer->configuration_failed(conf, e);
        throw;
    }
}

void ms::MediatingDisplayChanger::apply_base_config()
{
    apply_config(base_configuration_);
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
        try
        {
            apply_config(it->second);
        }
        catch (std::exception const&)
        {
            session->send_error(DisplayConfigurationFailedError{});
        }
    }
    else if (!base_configuration_applied)
    {
        // TODO: What happens if this fails?
        apply_base_config();
    }
}

void ms::MediatingDisplayChanger::no_focus_handler()
{
    std::lock_guard<std::mutex> lg{configuration_mutex};

    focused_session.reset();
    if (!base_configuration_applied)
    {
        apply_base_config();
    }
}

void ms::MediatingDisplayChanger::session_stopping_handler(
    std::shared_ptr<ms::Session> const& session)
{
    std::lock_guard<std::mutex> lg{configuration_mutex};

    config_map.erase(session);
}

void ms::MediatingDisplayChanger::set_base_configuration(std::shared_ptr<mg::DisplayConfiguration> const &conf)
{
    if (!conf->valid())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(bad_display_config));
    }

    server_action_queue->enqueue(
        this,
        [this, conf]
        {
            std::lock_guard<std::mutex> lg{configuration_mutex};

            base_configuration_ = conf;
            if (base_configuration_applied)
                apply_base_config();

            observer->base_configuration_updated(conf);
            send_config_to_all_sessions(conf);
        });
}

