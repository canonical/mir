/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "timeout_application_not_responding_detector.h"

#include "mir/time/alarm_factory.h"

#include <iostream>

namespace ms = mir::scene;
namespace mt = mir::time;

ms::TimeoutApplicationNotRespondingDetector::TimeoutApplicationNotRespondingDetector(
    mt::AlarmFactory& alarms,
    std::chrono::milliseconds period)
    : alarm{alarms.create_alarm([this, period]()
          {
              for (auto const& session_pair : sessions)
              {
                  session_pair.second();
              }
              this->alarm->reschedule_in(period);
          })}
{
    alarm->reschedule_in(period);
}

ms::TimeoutApplicationNotRespondingDetector::~TimeoutApplicationNotRespondingDetector()
{
}

void ms::TimeoutApplicationNotRespondingDetector::register_session(
    Session const& session, std::function<void()> const& pinger)
{
    sessions[&session] = pinger;
}

void ms::TimeoutApplicationNotRespondingDetector::unregister_session(
    Session const& session)
{
    sessions.erase(&session);
}

void ms::TimeoutApplicationNotRespondingDetector::pong_received(
   Session const& /*received_for*/)
{
}
