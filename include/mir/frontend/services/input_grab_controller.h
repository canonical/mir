/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_FRONTEND_SERVICES_INPUT_GRAB_CONTROLLER_H_
#define MIR_FRONTEND_SERVICES_INPUT_GRAB_CONTROLLER_H_

#include <memory>

namespace mir
{

class Application;

namespace frontend
{
namespace services
{

class InputGrabController
{
 public:
    virtual ~InputGrabController() {}

    virtual void grab_input_for_application(std::weak_ptr<Application> app) = 0;
    virtual std::weak_ptr<Application> get_grabbing_application() = 0;

    virtual void release_grab() = 0;

 protected:
    InputGrabController() = default;
    InputGrabController(const InputGrabController&) = delete;
    InputGrabController& operator=(const InputGrabController&) = delete;
};

}
}
}

#endif // MIR_FRONTEND_SERVICES_INPUT_GRAB_CONTROLLER_H_
