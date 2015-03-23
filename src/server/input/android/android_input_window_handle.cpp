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

#define MIR_LOG_COMPONENT "AndroidWindowHandle"
#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#include "android_input_window_handle.h"
#include "android_input_application_handle.h"

#include "mir/input/input_channel.h"
#include "mir/input/input_sender.h"
#include "mir/input/surface.h"
#include "mir/input/android/android_input_lexicon.h"

#include "mir/log.h"

#include <androidfw/InputTransport.h>

#include <limits.h>

namespace mi = mir::input;
namespace mia = mi::android;
namespace geom = mir::geometry;
namespace droidinput = android;

namespace
{
struct WindowInfo : public droidinput::InputWindowInfo
{
    WindowInfo(mi::Surface const* surface)
        : surface(surface)
    {
    }

    // Awkwardly the input stack hands us local coordinates here.
    bool touchableRegionContainsPoint(int32_t x, int32_t y) const override
    {
        return surface->input_area_contains({x, y});
    }

    mi::Surface const* surface;
};
}

mia::InputWindowHandle::InputWindowHandle(std::shared_ptr<mi::InputSender> const& input_sender,
                                          droidinput::sp<droidinput::InputApplicationHandle> const& input_app_handle,
                                          std::shared_ptr<mi::InputChannel> const& channel,
                                          mi::Surface const* surface)
  : droidinput::InputWindowHandle(input_app_handle),
    input_sender(input_sender),
    input_channel(channel),
    surface(surface)
{
    updateInfo();
}

bool mia::InputWindowHandle::updateInfo()
{
    if (!mInfo)
    {
        mInfo = new WindowInfo(surface);

        // TODO: How can we avoid recreating the InputChannel which the InputChannelFactory has already created?
        mInfo->inputChannel = new droidinput::InputChannel(droidinput::String8("TODO: Name"),
                                                           input_channel->server_fd());
    }

    auto const& bounds = surface->input_bounds();

    mInfo->frameLeft = bounds.top_left.x.as_uint32_t();
    mInfo->frameTop = bounds.top_left.y.as_uint32_t();
    mInfo->frameRight = mInfo->frameLeft + bounds.size.width.as_uint32_t();
    mInfo->frameBottom = mInfo->frameTop + bounds.size.height.as_uint32_t();

    mInfo->touchableRegionLeft = mInfo->frameLeft;
    mInfo->touchableRegionTop = mInfo->frameTop;
    mInfo->touchableRegionRight = mInfo->frameRight;
    mInfo->touchableRegionBottom = mInfo->frameBottom;

    mInfo->name = droidinput::String8(surface->name().c_str());
    mInfo->layoutParamsFlags = droidinput::InputWindowInfo::FLAG_NOT_TOUCH_MODAL;
    mInfo->layoutParamsType = droidinput::InputWindowInfo::TYPE_APPLICATION;
    mInfo->scaleFactor = 1.f;
    mInfo->visible = true;
    mInfo->canReceiveKeys = true;
    mInfo->hasFocus = true;
    mInfo->hasWallpaper = false;
    mInfo->paused = false;
    mInfo->dispatchingTimeout = std::chrono::nanoseconds(INT_MAX);
    mInfo->ownerPid = 0;
    mInfo->ownerUid = 0;
    mInfo->inputFeatures = 0;

    return true;
}

int64_t mia::InputWindowHandle::publishMotionEvent(int32_t deviceId,
    int32_t source,
    int32_t action,
    int32_t flags,
    int32_t edgeFlags,
    int32_t metaState,
    int32_t buttonState,
    float xOffset,
    float yOffset,
    float xPrecision,
    float yPrecision,
    std::chrono::nanoseconds downTime,
    std::chrono::nanoseconds eventTime,
    size_t pointerCount,
    droidinput::PointerProperties const* pointerProperties,
    droidinput::PointerCoords const* pointerCoords)
{
    droidinput::MotionEvent droid_event;
    droid_event.initialize(deviceId, source, action,flags,
                           edgeFlags, metaState, buttonState,
                           xOffset, yOffset, xPrecision, yPrecision,
                           downTime, eventTime,
                           pointerCount, pointerProperties, pointerCoords);

    MirEvent mir_event;
    mia::Lexicon::translate(&droid_event, mir_event);
    try {
        return input_sender->send_event(mir_event, input_channel);
    } catch (std::exception const& ex) {
        mir::log_error("Exception sending event to surface (%s): %s", surface->name().c_str(),
                 ex.what());
        return -1;
    }
}

int64_t mia::InputWindowHandle::publishKeyEvent(
    int32_t deviceId,
    int32_t source,
    int32_t action,
    int32_t flags,
    int32_t keyCode,
    int32_t scanCode,
    int32_t metaState,
    int32_t repeatCount,
    std::chrono::nanoseconds downTime,
    std::chrono::nanoseconds eventTime)
{
    droidinput::KeyEvent droid_event;
    droid_event.initialize(deviceId, source, action,flags, keyCode, scanCode, metaState, repeatCount, downTime, eventTime);

    MirEvent mir_event;
    mia::Lexicon::translate(&droid_event, mir_event);
    try
    {
        return input_sender->send_event(mir_event, input_channel);
    }
    catch (std::exception const& ex)
    {
        mir::log_error("Exception sending event to surface (%s): %s", surface->name().c_str(),
                 ex.what());
        return -1;
    }
}
