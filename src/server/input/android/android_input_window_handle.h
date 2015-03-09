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

#ifndef MIR_INPUT_ANDROID_INPUT_WINDOW_HANDLE_H_
#define MIR_INPUT_ANDROID_INPUT_WINDOW_HANDLE_H_

#include <androidfw/Input.h>
#include <InputWindow.h>

#include <memory>

namespace droidinput = android;

namespace mir
{

namespace input
{
class Surface;
class InputChannel;
class InputSender;

namespace android
{

class InputWindowHandle : public droidinput::InputWindowHandle
{
public:
    InputWindowHandle(std::shared_ptr<input::InputSender> const& input_sender,
                      droidinput::sp<droidinput::InputApplicationHandle> const& input_app_handle,
                      std::shared_ptr<input::InputChannel> const& channel,
                      input::Surface const* surface);
    ~InputWindowHandle() {}

    bool updateInfo();

    int64_t publishMotionEvent(int32_t deviceId,
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
        droidinput::PointerCoords const* pointerCoords);

    int64_t publishKeyEvent(
        int32_t deviceId,
        int32_t source,
        int32_t action,
        int32_t flags,
        int32_t keyCode,
        int32_t scanCode,
        int32_t metaState,
        int32_t repeatCount,
        std::chrono::nanoseconds downTime,
        std::chrono::nanoseconds eventTime);

protected:
    InputWindowHandle(InputWindowHandle const&) = delete;
    InputWindowHandle& operator=(InputWindowHandle const&) = delete;

private:
    std::shared_ptr<input::InputSender> const input_sender;
    std::shared_ptr<input::InputChannel> const input_channel;
    input::Surface const* surface;
};

}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_INPUT_WINDOW_HANDLE_H_
