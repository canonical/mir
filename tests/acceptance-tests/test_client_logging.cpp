/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test_framework/stub_client_connection_configuration.h" // XXX
#include "mir_test_framework/using_client_platform.h" // XXX
#include "mir_test_framework/connected_client_headless_server.h"
#include "mir/logging/logger.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace mir_test_framework;

namespace
{
class StringStreamLogger : public mir::logging::Logger
{
public:
    void log(mir::logging::Severity,
             std::string const& message,
             std::string const& component) override
    {
        ss << "[StringStreamLogger] "
           << component << ": " << message << std::endl;
    }
    static std::stringstream ss;
};

std::stringstream StringStreamLogger::ss;

class Conf : public StubConnectionConfiguration
{
public:
    Conf(std::string const& socket) :
        StubConnectionConfiguration(socket)
    {
    }
    virtual std::shared_ptr<mir::logging::Logger> the_logger() override
    {
        return std::make_shared<StringStreamLogger>();
    }
};

struct ClientLogging : ConnectedClientHeadlessServer
{
    UsingClientPlatform<Conf> platform;

    void SetUp() override
    {
        ConnectedClientHeadlessServer::SetUp();
    }

    std::stringstream& client_log{StringStreamLogger::ss};
};
} // namespace

TEST_F(ClientLogging, reports_performance)
{
    TemporaryEnvironmentValue env("MIR_CLIENT_PERF_REPORT", "log");
    (void)env; // Avoid clang warning/error

    auto spec = mir_connection_create_spec_for_normal_surface(
                   connection, 123, 456, mir_pixel_format_abgr_8888);
    ASSERT_THAT(spec, NotNull());
    mir_surface_spec_set_name(spec, "Foo");
    mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_software);
    auto surf = mir_surface_create_sync(spec);
    ASSERT_THAT(surf, NotNull());
    mir_surface_spec_release(spec);

    int const target_fps = 10;
    int const nseconds = 3;
    auto bs = mir_surface_get_buffer_stream(surf);
    for (int s = 0; s < nseconds; ++s)
    {
        for (int f = 0; f < target_fps; ++f)
            mir_buffer_stream_swap_buffers_sync(bs);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    int reports = 0;
    while (!client_log.eof())
    {
        std::string line;
        fprintf(stderr, "Here\n");
        std::getline(client_log, line);
        auto perf = line.find(" perf: ");
        if (perf != line.npos)
        {
            ++reports;
            char name[256];
            float fps;
            int fields = sscanf(line.c_str() + perf,
                                " perf: %255[^:]: %f FPS,", name, &fps);
            ASSERT_EQ(2, fields) << "Log line = {" << line << "}";
            EXPECT_STREQ("Foo", name);
            EXPECT_NEAR(target_fps, fps, 3.0f);
        }
        fprintf(stderr, "There\n");
    }

    EXPECT_THAT(reports, Ge(nseconds-1));

    fprintf(stderr, "aaa\n");
    mir_surface_release_sync(surf);
    fprintf(stderr, "aaa\n");
}
