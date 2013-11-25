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

#ifndef MIR_INPUT_ANDROID_WINDOW_HANDLE_REPOSITORY_H_
#define MIR_INPUT_ANDROID_WINDOW_HANDLE_REPOSITORY_H_

#include <utils/StrongPointer.h>

#include <memory>

namespace android
{
class InputWindowHandle;
}

namespace droidinput = android;

namespace mir
{
namespace input
{
class InputChannel;
namespace android
{

/// Interface internal to mir::input::android used for tracking the assosciation between droidinput::InputWindowHandle
/// and mir::input::InputChannel
class WindowHandleRepository
{
public:
    virtual ~WindowHandleRepository() = default;

    virtual droidinput::sp<droidinput::InputWindowHandle> handle_for_channel(std::shared_ptr<input::InputChannel const> const& channel) = 0;
protected:
    WindowHandleRepository() = default;
    WindowHandleRepository(const WindowHandleRepository&) = delete;
    WindowHandleRepository& operator=(const WindowHandleRepository&) = delete;
};

}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_WINDOW_HANDLE_REPOSITORY_H_
