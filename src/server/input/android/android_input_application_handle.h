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

#ifndef MIR_INPUT_ANDROID_INPUT_APPLICATION_HANDLE_H_
#define MIR_INPUT_ANDROID_INPUT_APPLICATION_HANDLE_H_

#include <InputApplication.h>

#include <memory>

namespace droidinput = android;

namespace mir
{

namespace input
{
class Surface;
namespace android
{

class InputApplicationHandle : public droidinput::InputApplicationHandle
{
public:
    InputApplicationHandle(input::Surface const* surface);
    ~InputApplicationHandle() {}

    bool updateInfo();

protected:
    InputApplicationHandle(InputApplicationHandle const&) = delete;
    InputApplicationHandle& operator=(InputApplicationHandle const&) = delete;

private:
    input::Surface const* surface;
};

}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_INPUT_APPLICATION_HANDLE_H_
