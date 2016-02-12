/*
 * Copyright © 2014-2016 Canonical Ltd.
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
 */

#include "mir_test_framework/async_server_runner.h"
#include "mir/test/popen.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <string>

namespace mo = mir::options;
namespace mtf = mir_test_framework;

namespace
{
struct GLMark2Test : testing::Test, mtf::AsyncServerRunner
{
    void SetUp() override
    {
        start_server();
    }

    void TearDown() override
    {
        stop_server();
    }

    enum ResultFileType {raw, json};
    virtual int run_glmark2(char const* args)
    {
        ResultFileType file_type = raw; // Should this still be selectable?

        char const* selection = getenv("MIR_GLMARK2_TEST_QUICK") ?
                                "-b build " : "";

        auto const cmd = "MIR_SOCKET=" + new_connection()
                       + " glmark2-es2-mir "
                       + selection
                       + args;
        mir::test::Popen p(cmd);

        const ::testing::TestInfo* const test_info =
          ::testing::UnitTest::GetInstance()->current_test_info();

        char output_filename[256];
        snprintf(output_filename, sizeof(output_filename) - 1,
                 "/tmp/%s_%s.log",
                 test_info->test_case_name(), test_info->name());

        printf("Saving GLMark2 detailed results to: %s\n", output_filename);
        // ^ Although I would vote to just print them to stdout instead

        std::string line;
        std::ofstream glmark2_output;
        int score = -1;
        glmark2_output.open(output_filename);
        while (p.get_line(line))
        {
            int match;
            if (sscanf(line.c_str(), " glmark2 Score: %d", &match) == 1)
            {
                score = match;
            }

            if (file_type == raw)
            {
                glmark2_output << line << std::endl;
            }
        }
        
        if (file_type == json)
        {
            std::string json =  "{";
                json += "\"benchmark_name\":\"glmark2-es2-mir\"";
                json += ",";
                json += "\"score\":\"" + std::to_string(score) + "\"";
                json += "}";
            glmark2_output << json;
        }

        return score;
    }
};

TEST_F(GLMark2Test, fullscreen_default)
{
    EXPECT_THAT(run_glmark2("--fullscreen"), ::testing::Ge(56));
}

#if 0
/*
 * FIXME: These all fail because the test server doesn't start more than once:
 *        "std::exception::what: Exception while creating graphics platform
 *        Exiting Mir! Reason: Nested Mir and Host Mir cannot use the same
 *        socket file to accept connections!"
 */
TEST_F(GLMark2Test, windowed_default)
{
    EXPECT_THAT(run_glmark2(""), ::testing::Ge(56));
}

TEST_F(GLMark2Test, fullscreen_interval1)
{
    add_to_environment("MIR_CLIENT_FORCE_SWAP_INTERVAL", "1");
    // Our devices seem to range 57-67Hz
    EXPECT_NEAR(60, run_glmark2("--fullscreen"), 10);
}

TEST_F(GLMark2Test, windowed_interval1)
{
    add_to_environment("MIR_CLIENT_FORCE_SWAP_INTERVAL", "1");
    // Our devices seem to range 57-67Hz
    EXPECT_NEAR(60, run_glmark2(""), 10);
}

#ifdef ANDROID
TEST_F(GLMark2Test, DISABLED_fullscreen_interval0) // LP: #1369763
#else
TEST_F(GLMark2Test, fullscreen_interval0)
#endif
{
    add_to_environment("MIR_CLIENT_FORCE_SWAP_INTERVAL", "0");
    EXPECT_THAT(run_glmark2("--fullscreen"), ::testing::Ge(100));
}

#ifdef ANDROID
TEST_F(GLMark2Test, DISABLED_windowed_interval0) // LP: #1369763
#else
TEST_F(GLMark2Test, windowed_interval0)
#endif
{
    add_to_environment("MIR_CLIENT_FORCE_SWAP_INTERVAL", "0");
    EXPECT_THAT(run_glmark2(""), ::testing::Ge(100));
}
#endif

}
