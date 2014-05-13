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

#ifndef MIR_INPUT_ANDROID_INPUT_CHANNEL_H_
#define MIR_INPUT_ANDROID_INPUT_CHANNEL_H_

#include "mir/input/input_channel.h"

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

class AndroidInputChannel : public InputChannel
{
public:
    explicit AndroidInputChannel();
    virtual ~AndroidInputChannel();

    int client_fd() const;
    int server_fd() const;

protected:
    AndroidInputChannel(AndroidInputChannel const&) = delete;
    AndroidInputChannel& operator=(AndroidInputChannel const&) = delete;

private:
    int s_fd, c_fd;
};

}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_INPUT_CHANNEL_H_
