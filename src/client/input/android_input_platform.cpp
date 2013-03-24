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

#include "android_input_platform.h"
#include "android_input_receiver.h"
#include "android_input_receiver_thread.h"

namespace mcl = mir::client;
namespace miat = mir::input::android::transport;

mcl::AndroidInputPlatform::AndroidInputPlatform()
{
}

mcl::AndroidInputPlatform::~AndroidInputPlatform()
{
}

std::shared_ptr<mcl::InputReceiverThread> mcl::AndroidInputPlatform::create_input_thread(
    int fd, std::function<void(MirEvent*)> callback)
{
    auto receiver = std::make_shared<miat::InputReceiver>(fd);
    return std::make_shared<miat::InputReceiverThread>(receiver, callback);
}
