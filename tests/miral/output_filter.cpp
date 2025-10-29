/*
* Copyright © Canonical Ltd.
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

#include <miral/output_filter.h>

#include <miral/live_config_ini_file.h>
#include <mir/logging/logger.h>
#include <mir/server.h>

#include <miral/test_server.h>
#include <mir/test/fake_shared.h>

#include <gmock/gmock.h>

using namespace testing;

namespace
{
struct OutputFilter : miral::TestServer
{
    OutputFilter()
    {
        start_server_in_setup = false;
    }

    struct TestLogger : mir::logging::Logger
    {
        MOCK_METHOD(void, log, (mir::logging::Severity severity, const std::string& message, const std::string& component), (override));
    };
    TestLogger test_logger;
};
}

TEST_F(OutputFilter, register_when_default_constructed)
{
    miral::OutputFilter output_filter;
    add_server_init([&output_filter](mir::Server& server) { output_filter(server); });
    start_server();
}

TEST_F(OutputFilter, register_with_live_config)
{
    miral::live_config::IniFile ini_file;
    miral::OutputFilter output_filter{ini_file};
    add_server_init([&output_filter](mir::Server& server) { output_filter(server); });
    start_server();
}

TEST_F(OutputFilter, register_when_constructed_with_none)
{
    miral::OutputFilter output_filter{MirOutputFilter::mir_output_filter_none};
    add_server_init([&output_filter](mir::Server& server) { output_filter(server); });
    start_server();
}

TEST_F(OutputFilter, register_when_constructed_with_grayscale)
{
    miral::OutputFilter output_filter{MirOutputFilter::mir_output_filter_grayscale};
    add_server_init([&output_filter](mir::Server& server) { output_filter(server); });
    start_server();
}

TEST_F(OutputFilter, register_when_constructed_with_invert)
{
    miral::OutputFilter output_filter{MirOutputFilter::mir_output_filter_invert};
    add_server_init([&output_filter](mir::Server& server) { output_filter(server); });
    start_server();
}

TEST_F(OutputFilter, live_config_can_set_none_without_warning)
{
    EXPECT_CALL(test_logger, log(mir::logging::Severity::warning, HasSubstr("output_filter"), _)).Times(0);
    EXPECT_CALL(test_logger, log(_, Not(HasSubstr("output_filter")), _)).Times(AnyNumber());

    auto const logger_builder = [this]() -> std::shared_ptr<mir::logging::Logger> { return mir::test::fake_shared(test_logger); };
    miral::live_config::IniFile ini_file;
    miral::OutputFilter output_filter{ini_file};
    add_server_init([&output_filter, &logger_builder](mir::Server& server)
    {
        output_filter(server);
        server.override_the_logger(logger_builder);
    });
    add_to_environment("MIR_SERVER_LOGGING", "true");
    start_server();


    std::istringstream input_stream{"output_filter=none"};
    ini_file.load_file(input_stream, "a_path");
}
