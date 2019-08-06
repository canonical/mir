/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "timeout_application_not_responding_detector.h"
#include "mir/frontend/session.h"
#include "mir/scene/session.h"

#include "mir/time/alarm_factory.h"

namespace ms = mir::scene;
namespace mt = mir::time;

struct ms::TimeoutApplicationNotRespondingDetector::ANRContext
{
    ANRContext(std::function<void()> const& pinger)
        : pinger{pinger},
          replied_since_last_ping{true},
          flagged_as_unresponsive{false}
    {
    }

    std::function<void()> const pinger;
    bool replied_since_last_ping;
    bool flagged_as_unresponsive;
};

void ms::TimeoutApplicationNotRespondingDetector::ANRObservers::session_unresponsive(
    Session const* session)
{
    for_each([session](auto const& observer) { observer->session_unresponsive(session); });
}

void ms::TimeoutApplicationNotRespondingDetector::ANRObservers::session_now_responsive(
    Session const* session)
{
    for_each([session](auto const& observer) { observer->session_now_responsive(session); });
}

ms::TimeoutApplicationNotRespondingDetector::TimeoutApplicationNotRespondingDetector(
    mt::AlarmFactory& alarms,
    std::chrono::milliseconds period)
    : period{period},
      alarm{alarms.create_alarm(std::bind(&TimeoutApplicationNotRespondingDetector::handle_ping_cycle, this))}
{
}

ms::TimeoutApplicationNotRespondingDetector::~TimeoutApplicationNotRespondingDetector()
{
}

void ms::TimeoutApplicationNotRespondingDetector::register_session(
    scene::Session const* session, std::function<void()> const& pinger)
{
    bool alarm_needs_schedule;
    {
        std::lock_guard<std::mutex> lock{session_mutex};
        sessions[dynamic_cast<Session const*>(session)] = std::make_unique<ANRContext>(pinger);
        alarm_needs_schedule = alarm->state() != mt::Alarm::State::pending;
    }
    if (alarm_needs_schedule)
    {
        alarm->reschedule_in(period);
    }
}

void ms::TimeoutApplicationNotRespondingDetector::unregister_session(
    scene::Session const* session)
{
    std::lock_guard<std::mutex> lock{session_mutex};
    sessions.erase(dynamic_cast<Session const*>(session));
}

void ms::TimeoutApplicationNotRespondingDetector::pong_received(
   scene::Session const* received_for)
{
    bool needs_now_responsive_notification{false};
    bool alarm_needs_rescheduling;
    {
        std::lock_guard<std::mutex> lock{session_mutex};

        auto& session_ctx = sessions.at(dynamic_cast<Session const*>(received_for));
        if (session_ctx->flagged_as_unresponsive)
        {
            session_ctx->flagged_as_unresponsive = false;
            needs_now_responsive_notification = true;
        }
        session_ctx->replied_since_last_ping = true;

        alarm_needs_rescheduling = alarm->state() != mt::Alarm::State::pending;
    }
    if (needs_now_responsive_notification)
    {
        observers.session_now_responsive(dynamic_cast<Session const*>(received_for));
    }
    if (alarm_needs_rescheduling)
    {
        alarm->reschedule_in(period);
    }
}

void ms::TimeoutApplicationNotRespondingDetector::register_observer(
    std::shared_ptr<Observer> const& observer)
{
    observers.add(observer);

    std::vector<Session const*> unresponsive_sessions;
    {
        std::lock_guard<std::mutex> lock{session_mutex};
        for (auto const& session_pair : sessions)
        {
            if (session_pair.second->flagged_as_unresponsive)
            {
                unresponsive_sessions.push_back(session_pair.first);
            }
        }
    }

    for (auto const& unresponsive_session : unresponsive_sessions)
    {
       observer->session_unresponsive(unresponsive_session);
    }
}

void ms::TimeoutApplicationNotRespondingDetector::unregister_observer(
    std::shared_ptr<Observer> const& observer)
{
    observers.remove(observer);
}

void ms::TimeoutApplicationNotRespondingDetector::handle_ping_cycle()
{
    bool needs_rearm{false};
    {
        std::lock_guard<std::mutex> lock{session_mutex};
        for (auto const& session_pair : sessions)
        {
            bool const newly_unresponsive =
                !session_pair.second->replied_since_last_ping &&
                !session_pair.second->flagged_as_unresponsive;
            bool const needs_ping =
                session_pair.second->replied_since_last_ping;

            if (newly_unresponsive)
            {
                session_pair.second->flagged_as_unresponsive = true;
                unresponsive_sessions_temporary.push_back(session_pair.first);
            }
            else if (needs_ping)
            {
                session_pair.second->pinger();
                session_pair.second->replied_since_last_ping = false;
                needs_rearm = true;
            }
        }
    }

    // Dispatch notifications outside the lock.
    for (auto const& unresponsive_session : unresponsive_sessions_temporary)
    {
        observers.session_unresponsive(unresponsive_session);
    }

    unresponsive_sessions_temporary.clear();

    if (needs_rearm)
    {
        this->alarm->reschedule_in(this->period);
    }
}
