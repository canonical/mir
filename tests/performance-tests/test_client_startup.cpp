/*
 * Copyright © 2016 Canonical Ltd.
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
#include "mir_toolkit/mir_client_library.h"

#include "mir/test/validity_matchers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>

namespace mtf = mir_test_framework;

using namespace testing;

namespace
{
struct ClientStartupPerformance : testing::Test, mtf::AsyncServerRunner
{
    void SetUp() override
    {
        start_server();
    }

    void TearDown() override
    {
        stop_server();
    }

    MirConnection* create_connection()
    {
        auto conn = mir_connect_sync(new_connection().c_str(), "Perf test");
        if (!mir_connection_is_valid(conn))
        {
            std::string error_msg{"Could not create connection: "};
            error_msg.append(mir_connection_get_error_message(conn));
            throw std::runtime_error(error_msg);
        }
        return conn;
    }
};

MirPixelFormat find_pixel_format(MirConnection* connection)
{
    MirPixelFormat pixel_format = mir_pixel_format_invalid;
    unsigned int valid_formats;
    mir_connection_get_available_surface_formats(connection, &pixel_format, 1, &valid_formats);
    if (valid_formats < 1 || pixel_format == mir_pixel_format_invalid)
        throw std::runtime_error("Could not find pixel format");
    return pixel_format;
}

MirSurface* make_surface(MirConnection* connection)
{
    auto format = find_pixel_format(connection);

    auto spec = mir_connection_create_spec_for_normal_surface(connection,
            720, 1280, format);
    auto surface = mir_surface_create_sync(spec);

    mir_surface_spec_release(spec);

    if (!mir_surface_is_valid(surface))
    {
        std::string error_msg{"Could not create surface: "};
        error_msg.append(mir_surface_get_error_message(surface));
        throw std::runtime_error(error_msg);
    }
    return surface;
}
}

TEST_F(ClientStartupPerformance, create_surface_and_swap)
{
    using namespace std::chrono_literals;
    auto start = std::chrono::steady_clock::now();

    auto conn = create_connection();
    auto surf = make_surface(conn);
    auto stream  = mir_surface_get_buffer_stream(surf);
    if (!mir_buffer_stream_is_valid(stream))
    {
        std::string error_msg{"Could not get buffer stream from surface: "};
        error_msg.append(mir_buffer_stream_get_error_message(stream));
        throw std::runtime_error(error_msg);
    }

    mir_buffer_stream_swap_buffers_sync(stream);

    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);

    //NOTE: Ideally, the expected number should vary according to platform
    auto max_expected_time = 80ms;
    EXPECT_THAT(diff.count(), Lt(max_expected_time.count()));

    mir_surface_release_sync(surf);
    mir_connection_release(conn);
}

