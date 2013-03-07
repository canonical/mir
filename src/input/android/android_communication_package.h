/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_ANDROID_COMMUNICATION_PACKAGE_H_
#define MIR_INPUT_ANDROID_COMMUNICATION_PACKAGE_H_

#include "mir/input/communication_package.h"

#include <utils/StrongPointer.h>

namespace android
{
class InputChannel;
}

namespace droidinput = android;

namespace mir
{
namespace input
{
namespace android
{

class AndroidCommunicationPackage : public CommunicationPackage
{
public:
    explicit AndroidCommunicationPackage();
    virtual ~AndroidCommunicationPackage();
    
    int client_fd() const;
    int server_fd() const;

protected:
    AndroidCommunicationPackage(AndroidCommunicationPackage const&) = delete;
    AndroidCommunicationPackage& operator=(AndroidCommunicationPackage const&) = delete;

private:
    droidinput::sp<droidinput::InputChannel> client_channel;
    droidinput::sp<droidinput::InputChannel> server_channel;
};

}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_COMMUNICATION_PACKAGE_H_
