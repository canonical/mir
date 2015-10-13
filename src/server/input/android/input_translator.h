/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_ANDROID_INPUT_TRANSLATOR_H_
#define MIR_INPUT_ANDROID_INPUT_TRANSLATOR_H_

#include "mir/input/input_dispatcher.h"

#include "InputListener.h"

#include <memory>

namespace droidinput = android;

namespace mir
{
namespace cookie
{
class CookieFactory;
}
namespace input
{
class InputDispatcher;
namespace android
{
class InputTranslator : public droidinput::InputListenerInterface
{
public:
    InputTranslator(std::shared_ptr<InputDispatcher> const& dispatcher,
                    std::shared_ptr<cookie::CookieFactory> const& cookie_factory);

    void notifyConfigurationChanged(const droidinput::NotifyConfigurationChangedArgs* args) override;
    void notifyKey(const droidinput::NotifyKeyArgs* args) override;
    void notifyMotion(const droidinput::NotifyMotionArgs* args) override;
    void notifySwitch(const droidinput::NotifySwitchArgs* args) override;
    void notifyDeviceReset(const droidinput::NotifyDeviceResetArgs* args) override;

private:
    std::shared_ptr<InputDispatcher> const dispatcher;
    std::shared_ptr<cookie::CookieFactory> const cookie_factory;
};

}
}
}

#endif
