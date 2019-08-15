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

#ifndef MIR_TEST_DOUBLES_NULL_APPLICATION_NOT_RESPONDING_DETECTOR_H_
#define MIR_TEST_DOUBLES_NULL_APPLICATION_NOT_RESPONDING_DETECTOR_H_

#include "mir/scene/application_not_responding_detector.h"

namespace mir
{
namespace test
{
namespace doubles
{
class NullANRDetector : public mir::scene::ApplicationNotRespondingDetector
{
public:
    void register_session(scene::Session const*, std::function<void()> const&) override
    {
    }
    void unregister_session(scene::Session const*) override
    {
    }
    void pong_received(scene::Session const*) override
    {
    }
    void register_observer(std::shared_ptr<Observer> const&) override
    {
    }
    void unregister_observer(std::shared_ptr<Observer> const&) override
    {
    }
};
}
}
}

#endif // MIR_TEST_DOUBLES_NULL_APPLICATION_NOT_RESPONDING_DETECTOR_H_
