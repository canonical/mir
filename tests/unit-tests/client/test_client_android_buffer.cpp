/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_client/android_client_buffer.h"

#include <memory>
#include <gtest/gtest.h>

namespace mcl=mir::client;

class ClientAndroidBufferTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }
};

TEST_F(ClientAndroidBufferTest, client_init)
{
    mcl::MirBufferPackage package ;
    auto buffer = std::make_shared<mcl::AndroidClientBuffer>(package);
    EXPECT_NE((int) buffer.get(), NULL);
}

