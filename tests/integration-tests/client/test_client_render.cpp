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

#include "mir/process/process.h"

#include <gmock/gmock.h>

namespace mp=mir::process;

struct TestClient
{

/* client code */
static int main_function()
{
    /* only use C */

    /* make surface */
    /* grab a buffer*/
    /* render pattern */
    /* release */

    /* exit */
    return 0;
}

static int exit_function()
{
    return 0;
}

};



struct TestClientIPCRender : public testing::Test
{

    void SetUp() {

    }
};


TEST_F(TestClientIPCRender, test_render)
{
    /* start server */
    auto p = mp::fork_and_run_in_a_different_process(
        TestClient::main_function,
        TestClient::exit_function);

    /* wait for connect */    
    /* wait for buffer sent back */

    EXPECT_TRUE(p->wait_for_termination().succeeded());


    /* verify pattern */
}
