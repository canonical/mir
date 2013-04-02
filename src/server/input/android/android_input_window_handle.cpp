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

#include "android_input_window_handle.h"
#include "android_input_application_handle.h"

#include "mir/input/input_channel.h"
#include "mir/input/surface_target.h"

#include <androidfw/InputTransport.h>

#include <limits.h>

namespace mi = mir::input;
namespace mia = mi::android;

mia::InputWindowHandle::InputWindowHandle(droidinput::sp<droidinput::InputApplicationHandle> const& input_app_handle,
                                          std::shared_ptr<mi::SurfaceTarget> const& surface)
  : droidinput::InputWindowHandle(input_app_handle),
    surface(surface)
{
    updateInfo();
}

bool mia::InputWindowHandle::updateInfo()
{
    if (!mInfo)
    {
        mInfo = new droidinput::InputWindowInfo();

        // TODO: How can we avoid recreating the InputChannel which the InputChannelFactory has already created?
        mInfo->inputChannel = new droidinput::InputChannel(droidinput::String8("TODO: Name"),
                                                           surface->server_input_fd());
    }

    mInfo->frameLeft = 0;
    mInfo->frameTop = 0;
    auto surface_size = surface->size();
    mInfo->frameRight = mInfo->frameLeft + surface_size.width.as_uint32_t();
    mInfo->frameBottom = mInfo->frameTop + surface_size.height.as_uint32_t();

    mInfo->name = droidinput::String8(surface->name().c_str());
    mInfo->layoutParamsFlags = droidinput::InputWindowInfo::FLAG_NOT_TOUCH_MODAL;
    mInfo->layoutParamsType = droidinput::InputWindowInfo::TYPE_APPLICATION;
    mInfo->scaleFactor = 1.f;
    mInfo->visible = true;
    mInfo->canReceiveKeys = true;
    mInfo->hasFocus = true;
    mInfo->hasWallpaper = false;
    mInfo->paused = false;
    mInfo->dispatchingTimeout = INT_MAX;
    mInfo->ownerPid = 0;
    mInfo->ownerUid = 0;
    mInfo->inputFeatures = 0;

    // TODO: Set touchableRegion and layer for touch events.

    return true;
}
