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

#include "mir_client/client_platform.h"

class ClientPlatformTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }

};

TEST_F(ClientPlatformTest, platform_creates )
{
    auto platform = mcl::create_client_platform(); 
    auto depository = platform->create_depository();
    
    EXPECT_NE(NULL, depository.get());
}

