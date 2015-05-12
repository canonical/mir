/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_CLIENT_ANDROID_INPUT_PLATFORM_H_
#define MIR_CLIENT_ANDROID_INPUT_PLATFORM_H_

#include "mir/input/input_platform.h"

namespace mir
{
namespace dispatch
{
class Dispatchable;
}
namespace input
{
namespace receiver
{
namespace android
{

/// Implementation of client input machinery for android input stack wire protocol.
class AndroidInputPlatform : public InputPlatform
{
public:
    AndroidInputPlatform(std::shared_ptr<InputReceiverReport> const& report);
    virtual ~AndroidInputPlatform();

    std::shared_ptr<dispatch::Dispatchable> create_input_receiver(int fd,
                                                                  std::shared_ptr<XKBMapper> const& mapper,
                                                                  std::function<void(MirEvent*)> const& callback);

protected:
    AndroidInputPlatform(const AndroidInputPlatform&) = delete;
    AndroidInputPlatform& operator=(const AndroidInputPlatform&) = delete;

private:
    std::shared_ptr<InputReceiverReport> const report;
};
}
}
}
}  // namespace mir

#endif  // MIR_CLIENT_ANDROID_INPUT_PLATFORM_H_
