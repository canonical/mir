/*
 * Copyright © 2013-2015 Canonical Ltd.
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

#include "src/server/input/android/android_input_channel.h"
#include "src/server/input/android/input_channel_factory.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <initializer_list>

namespace mia = mir::input::android;

TEST(AndroidInputChannelFactory, channel_factory_returns_input_channel_with_fds)
{
    mia::InputChannelFactory factory;

    auto package = factory.make_input_channel();
    EXPECT_NE(nullptr, std::dynamic_pointer_cast<mia::AndroidInputChannel>(package));
}
