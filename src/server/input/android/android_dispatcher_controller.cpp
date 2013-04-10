/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "android_dispatcher_controller.h"

#include "mir/input/android/android_input_configuration.h"

#include <InputDispatcher.h>

namespace mi = mir::input;
namespace mia = mi::android;

mia::DispatcherController::DispatcherController(std::shared_ptr<mia::InputConfiguration> const& config)
  : input_dispatcher(config->the_dispatcher())
{
}

void mia::DispatcherController::set_input_focus_to(std::shared_ptr<mi::SessionTarget> const& session,
    std::shared_ptr<mi::SurfaceTarget> const& surface)
{
    // TODO: Implement ~racarr
    (void) session;
    (void) surface;
}
