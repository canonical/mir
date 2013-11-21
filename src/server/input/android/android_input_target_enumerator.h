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

#ifndef MIR_INPUT_ANDROID_TARGET_ENUMERATOR_H_
#define MIR_INPUT_ANDROID_TARGET_ENUMERATOR_H_

#include <InputEnumerator.h>

#include <utils/StrongPointer.h>

#include <memory>
#include <functional>

namespace android
{
class InputWindowHandle;
}

namespace droidinput = android;

namespace mir
{
namespace scene
{
class InputRegitrar;
}
namespace input
{
class InputTargets;
namespace android
{
class WindowHandleRepository;

class InputTargetEnumerator : public droidinput::InputEnumerator
{
public:
    explicit InputTargetEnumerator(std::shared_ptr<input::InputTargets> const& targets,
                                   std::shared_ptr<WindowHandleRepository> const& repository);
    virtual ~InputTargetEnumerator() noexcept(true);
    
    void for_each(std::function<void(droidinput::sp<droidinput::InputWindowHandle> const&)> const& callback);
    
private:
    std::weak_ptr<input::InputTargets> const targets;
    std::weak_ptr<input::android::WindowHandleRepository> const repository;
};

}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_TARGET_ENUMERATOR_H_
