/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_TEST_SYSTEM_PERFORMANCE_TEST_H_
#define MIR_TEST_SYSTEM_PERFORMANCE_TEST_H_

#include <gtest/gtest.h>
#include <string>
#include <chrono>
#include <initializer_list>

namespace mir { namespace test {

class SystemPerformanceTest : public testing::Test
{
protected:
    SystemPerformanceTest();
    void set_up_with(std::string const server_args);
    void TearDown() override;
    void spawn_clients(std::initializer_list<std::string> clients);
    void run_server_for(std::chrono::seconds timeout);

    FILE* server_output;
private:
    std::string const bin_dir;
    pid_t server_pid = 0;
};

} } // namespace mir::test

#endif // MIR_TEST_SYSTEM_PERFORMANCE_TEST_H_
