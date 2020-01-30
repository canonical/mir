/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SCENE_APPLICATION_NOT_RESPONDING_DETECTOR_WRAPPER_H_
#define MIR_SCENE_APPLICATION_NOT_RESPONDING_DETECTOR_WRAPPER_H_

#include "mir/scene/application_not_responding_detector.h"

#include <memory>

namespace mir
{
namespace scene
{
class ApplicationNotRespondingDetectorWrapper : public ApplicationNotRespondingDetector
{
public:
    ApplicationNotRespondingDetectorWrapper(std::shared_ptr<ApplicationNotRespondingDetector> const& wrapped);
    ~ApplicationNotRespondingDetectorWrapper();

    virtual void register_session(scene::Session const* session, std::function<void()> const& pinger) override;
    virtual void unregister_session(scene::Session const* session) override;
    virtual void pong_received(scene::Session const* received_for) override;
    virtual void register_observer(std::shared_ptr<Observer> const& observer) override;
    virtual void unregister_observer(std::shared_ptr<Observer> const& observer) override;

protected:
    std::shared_ptr<ApplicationNotRespondingDetector> const wrapped;
};
}
}


#endif //MIR_SCENE_APPLICATION_NOT_RESPONDING_DETECTOR_WRAPPER_H_
