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

#include "android_input_platform.h"
#include "android_input_receiver.h"
#include "android_input_receiver_thread.h"

namespace mcli = mir::client::input;
namespace mclia = mcli::android;

mclia::AndroidInputPlatform::AndroidInputPlatform()
{
}

mclia::AndroidInputPlatform::~AndroidInputPlatform()
{
}

std::shared_ptr<mcli::InputReceiverThread> mclia::AndroidInputPlatform::create_input_thread(
    int fd, std::function<void(MirEvent*)> const& callback)
{
    auto receiver = std::make_shared<mclia::InputReceiver>(fd);
    return std::make_shared<mclia::InputReceiverThread>(receiver, callback);
}

std::shared_ptr<mcli::InputPlatform> mcli::InputPlatform::create()
{
    return std::make_shared<mclia::AndroidInputPlatform>();
}
