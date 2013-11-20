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

#ifndef MIR_INPUT_ANDROID_REGISTRAR_H_
#define MIR_INPUT_ANDROID_REGISTRAR_H_

#include "android_window_handle_repository.h"

#include "mir/scene/input_registrar.h"

#include <utils/StrongPointer.h>

#include <map>
#include <mutex>

namespace android
{
class InputDispatcherInterface;
class InputWindowHandle;
}

namespace droidinput = android;

namespace mir
{
namespace input
{
class Surface;
namespace android
{
class InputConfiguration;
class InputTargeter;

class InputRegistrar : public scene::InputRegistrar, public WindowHandleRepository
{
public:
    explicit InputRegistrar(droidinput::sp<droidinput::InputDispatcherInterface> const& input_dispatcher);
    virtual ~InputRegistrar() noexcept(true);

    void input_channel_opened(std::shared_ptr<input::InputChannel> const& opened_channel,
                              std::shared_ptr<input::Surface> const& surface,
                              InputReceptionMode mode);

    void input_channel_closed(std::shared_ptr<input::InputChannel> const& closed_channel);


    virtual droidinput::sp<droidinput::InputWindowHandle> handle_for_channel(std::shared_ptr<input::InputChannel const> const& channel);

private:
    droidinput::sp<droidinput::InputDispatcherInterface> const input_dispatcher;

    std::map<std::shared_ptr<input::InputChannel const>, droidinput::sp<droidinput::InputWindowHandle>> window_handles;

    std::mutex handles_mutex;
};

}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_REGISTRAR_H_
