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

#ifndef MIR_SCENE_APPLICATION_NOT_RESPONDING_DETECTOR_H_
#define MIR_SCENE_APPLICATION_NOT_RESPONDING_DETECTOR_H_

#include <memory>
#include <functional>

namespace mir
{
namespace scene
{
class Session;

class ApplicationNotRespondingDetector
{
public:
    ApplicationNotRespondingDetector() = default;
    virtual ~ApplicationNotRespondingDetector() = default;

    /**
     * Notification object for application-not-responsive signals
     *
     * \note These signals will be called without any locks held; it's safe
     *       to call methods on the ApplicationNotRespondingDetector from these
     *       delegates.
     */
    class Observer
    {
    public:
        Observer() = default;
        virtual ~Observer() = default;

        virtual void session_unresponsive(Session const* session) = 0;
        virtual void session_now_responsive(Session const* session) = 0;
    };

    virtual void register_session(scene::Session const* session, std::function<void()> const& pinger) = 0;
    virtual void unregister_session(scene::Session const* session) = 0;
    virtual void pong_received(scene::Session const* received_for) = 0;

    virtual void register_observer(std::shared_ptr<Observer> const& observer) = 0;
    virtual void unregister_observer(std::shared_ptr<Observer> const& observer) = 0;
};
}
}

#endif // MIR_SCENE_APPLICATION_NOT_RESPONDING_DETECTOR_H_
