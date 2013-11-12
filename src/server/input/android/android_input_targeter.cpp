/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "android_input_targeter.h"
#include "android_input_registrar.h"

#include "android_input_window_handle.h"
#include "android_input_application_handle.h"

#include <InputDispatcher.h>

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <mutex>

namespace mi = mir::input;
namespace mia = mi::android;
namespace ms = mir::surfaces;

mia::InputTargeter::InputTargeter(droidinput::sp<droidinput::InputDispatcherInterface> const& input_dispatcher,
                                  std::shared_ptr<mia::WindowHandleRepository> const& repository) :
    input_dispatcher(input_dispatcher),
    repository(repository)
{
}

mia::InputTargeter::~InputTargeter() noexcept(true) {}


void mia::InputTargeter::focus_cleared()
{
    droidinput::sp<droidinput::InputWindowHandle> null_window = nullptr;
    
    input_dispatcher->setKeyboardFocus(null_window);
}

void mia::InputTargeter::focus_changed(std::shared_ptr<mi::InputChannel const> const& focus_channel)
{
    auto window_handle = repository->handle_for_channel(focus_channel);
    
    if (window_handle == NULL)
        BOOST_THROW_EXCEPTION(std::logic_error("Attempt to set keyboard focus to an unregistered input channel"));
    
    input_dispatcher->setKeyboardFocus(window_handle);
}
