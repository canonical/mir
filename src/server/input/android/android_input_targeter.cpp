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

#include "mir/input/android/android_input_configuration.h"

#include <InputDispatcher.h>

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <mutex>

namespace mi = mir::input;
namespace mia = mi::android;
namespace ms = mir::surfaces;

mia::InputTargeter::InputTargeter(std::shared_ptr<mia::InputConfiguration> const& config,
                                  std::shared_ptr<ms::InputRegistrar> const& input_registrar) :
    input_dispatcher(config->the_dispatcher()),
    input_registrar(std::dynamic_pointer_cast<mia::InputRegistrar>(input_registrar))
{
    assert(input_registrar);
}

void mia::InputTargeter::focus_cleared()
{
    droidinput::Vector<droidinput::sp<droidinput::InputWindowHandle>> empty_windows;
    droidinput::sp<droidinput::InputApplicationHandle> null_application = nullptr;
    
    input_dispatcher->setFocusedApplication(null_application);
    input_dispatcher->setInputWindows(empty_windows);
}

void mia::InputTargeter::focus_changed(std::shared_ptr<mi::SurfaceTarget const> const& surface)
{
    auto window_handle = input_registrar->handle_for_surface(surface);

    if (!window_handle.get())
        BOOST_THROW_EXCEPTION(std::logic_error("Focus changed to an unopened surface"));
    auto application_handle = window_handle->inputApplicationHandle;
    
    input_dispatcher->setFocusedApplication(application_handle);

    droidinput::Vector<droidinput::sp<droidinput::InputWindowHandle>> windows;
    windows.push_back(window_handle);

    input_dispatcher->setInputWindows(windows);
}
