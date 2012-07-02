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

#include "mir/application.h"
#include "mir/application_manager.h"
#include "mir/input/dispatcher.h"

#include <cassert>
#include <memory>
#include <set>

namespace mi = mir::input;

mi::GrabFilter::GrabFilter(mir::ApplicationManager* application_manager) : application_manager(application_manager)
{
    assert(application_manager);
}

mi::Filter::Result mi::GrabFilter::accept(Event* e)
{
    assert(e);
        
    std::shared_ptr<mir::Application> input_grabber = application_manager->get_grabbing_application().lock();
    
    if (!input_grabber)
        return mi::Filter::Result::continue_processing;
    
    input_grabber->on_event(e);
    
    return mi::Filter::Result::stop_processing;
}
