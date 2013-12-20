/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "android_input_application_handle.h"

#include "mir/input/input_channel.h"
#include "mir/input/surface.h"

#include <limits.h>

namespace mi = mir::input;
namespace mia = mir::input::android;

mia::InputApplicationHandle::InputApplicationHandle(std::shared_ptr<mi::Surface> const& surface)
  : surface(surface)
{
    updateInfo();
}

bool mia::InputApplicationHandle::updateInfo()
{
    if (mInfo == NULL)
        mInfo = new droidinput::InputApplicationInfo;

    mInfo->dispatchingTimeout = INT_MAX;
    mInfo->name = droidinput::String8(surface->name().c_str());

    return true;
}
