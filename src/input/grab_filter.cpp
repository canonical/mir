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

#include "mir/input/grab_filter.h"

#include "mir/frontend/application.h"
#include "mir/frontend/services/input_grab_controller.h"
#include "mir/input/dispatcher.h"

#include <cassert>
#include <memory>
#include <set>

namespace mf = mir::frontend;
namespace mfs = mir::frontend::services;
namespace mi = mir::input;

mi::GrabFilter::GrabFilter(mfs::InputGrabController* controller) : input_grab_controller(controller)
{
    assert(input_grab_controller);
}

mi::Filter::Result mi::GrabFilter::accept(Event* e)
{
    assert(e);
        
    std::shared_ptr<mf::Application> input_grabber = input_grab_controller->get_grabbing_application().lock();
    
    if (!input_grabber)
        return mi::Filter::Result::continue_processing;
    
    input_grabber->on_event(e);
    
    return mi::Filter::Result::stop_processing;
}
