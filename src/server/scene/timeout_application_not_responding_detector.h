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

#ifndef MIR_SCENE_TIMOUT_APPLICATION_NOT_RESPONDING_DETECTOR_
#define MIR_SCENE_TIMOUT_APPLICATION_NOT_RESPONDING_DETECTOR_

#include "mir/scene/application_not_responding_detector.h"
#include "mir/basic_observers.h"

#include <chrono>
#include <unordered_map>
#include <functional>

namespace mir
{
namespace time
{
class Alarm;
class AlarmFactory;
}

namespace scene
{
class TimeoutApplicationNotRespondingDetector : public ApplicationNotRespondingDetector
{
public:
    TimeoutApplicationNotRespondingDetector(
        time::AlarmFactory& alarms,
        std::chrono::milliseconds period);

    template<typename Rep, typename Period>
    TimeoutApplicationNotRespondingDetector(
        time::AlarmFactory& alarms,
        std::chrono::duration<Rep, Period> period)
        : TimeoutApplicationNotRespondingDetector(alarms,
              std::chrono::duration_cast<std::chrono::milliseconds>(period))
    {
    }

    ~TimeoutApplicationNotRespondingDetector() override;

    void register_session(scene::Session const* session,
        std::function<void()> const& pinger) override;

    void unregister_session(scene::Session const* session) override;

    void pong_received(scene::Session const* received_for) override;

    void register_observer(std::shared_ptr<Observer> const& observer) override;
    void unregister_observer(std::shared_ptr<Observer> const& observer) override;
private:
    void handle_ping_cycle();

    struct ANRContext;

    class ANRObservers : public Observer, private BasicObservers<Observer>
    {
    public:
        using BasicObservers<Observer>::add;
        using BasicObservers<Observer>::remove;

        void session_unresponsive(Session const* session) override;
        void session_now_responsive(Session const* session) override;
    } observers;

    std::mutex session_mutex;
    std::unordered_map<Session const*, std::unique_ptr<ANRContext>> sessions;
    std::vector<Session const*> unresponsive_sessions_temporary;

    std::chrono::milliseconds const period;
    std::unique_ptr<time::Alarm> const alarm;
};
}
}

#endif // MIR_SCENE_TIMOUT_APPLICATION_NOT_RESPONDING_DETECTOR_
