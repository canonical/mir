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
#include "mir/input/surface_info.h"

#include <androidfw/InputTransport.h>

#include <limits.h>

namespace mi = mir::input;
namespace mia = mi::android;
namespace geom = mir::geometry;

namespace
{
struct WindowInfo : public droidinput::InputWindowInfo
{
    WindowInfo(std::shared_ptr<mi::SurfaceInfo> const& info)
        : info(info)
    {
    }

    bool touchableRegionContainsPoint(int32_t x, int32_t y) const override
    {
        return info->input_region_contains({geom::X{x}, geom::Y{y}});
    }
    
    std::shared_ptr<mi::SurfaceInfo> const info;
};
}

mia::InputWindowHandle::InputWindowHandle(droidinput::sp<droidinput::InputApplicationHandle> const& input_app_handle,
                                          std::shared_ptr<mi::InputChannel> const& channel,
                                          std::shared_ptr<mi::SurfaceInfo> const& info)
  : droidinput::InputWindowHandle(input_app_handle),
    input_channel(channel),
    surface_info(info)
{
    updateInfo();
}

bool mia::InputWindowHandle::updateInfo()
{
    if (!mInfo)
    {
        mInfo = new WindowInfo(surface_info);

        // TODO: How can we avoid recreating the InputChannel which the InputChannelFactory has already created?
        mInfo->inputChannel = new droidinput::InputChannel(droidinput::String8("TODO: Name"),
                                                           input_channel->server_fd());
    }

    auto surface_rect = surface_info->size_and_position();
    auto surface_position = surface_rect.top_left;
    auto surface_size = surface_rect.size;

    mInfo->frameLeft = surface_position.x.as_uint32_t();
    mInfo->frameTop = surface_position.y.as_uint32_t();
    mInfo->frameRight = mInfo->frameLeft + surface_size.width.as_uint32_t();
    mInfo->frameBottom = mInfo->frameTop + surface_size.height.as_uint32_t();
    
    mInfo->touchableRegionLeft = mInfo->frameLeft;
    mInfo->touchableRegionTop = mInfo->frameTop;
    mInfo->touchableRegionRight = mInfo->frameRight;
    mInfo->touchableRegionBottom = mInfo->frameBottom;

    mInfo->name = droidinput::String8(surface_info->name().c_str());
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
