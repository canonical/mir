/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_INPUT_GRAB_FILTER_H_
#define MIR_INPUT_GRAB_FILTER_H_

#include "mir/application.h"
#include "mir/application_manager.h"
#include "mir/input/dispatcher.h"

#include <memory>
#include <set>

namespace mir
{
namespace input
{

class GrabFilter : public Dispatcher::GrabFilter
{
 public:
    explicit GrabFilter(mir::ApplicationManager* application_manager) : application_manager(application_manager)
    {
        assert(application_manager);
    }

    Filter::Result accept(Event* e)
    {
        assert(e);
        
        std::shared_ptr<mir::Application> input_grabber = application_manager->get_grabbing_application().lock();

        if (!input_grabber)
            return Filter::Result::continue_processing;

        input_grabber->on_event(e);

        return Filter::Result::stop_processing;
    }

 private:
    mir::ApplicationManager* application_manager;
};

}
}

#endif // MIR_INPUT_GRAB_FILTER_H_
