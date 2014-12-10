/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/debug/surface.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/basic_client_server_fixture.h"
#include "mir_test_framework/any_surface.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <condition_variable>
#include <mutex>

namespace mtf = mir_test_framework;

namespace
{
struct SurfaceSync
{
    void surface_created(MirSurface * new_surface)
    {
        std::unique_lock<std::mutex> lock(guard);
        surface = new_surface;
        wait_condition.notify_all();
    }

    void surface_released(MirSurface * /*released_surface*/)
    {
        std::unique_lock<std::mutex> lock(guard);
        surface = nullptr;
        wait_condition.notify_all();
    }

    void wait_for_surface_create()
    {
        std::unique_lock<std::mutex> lock(guard);
        wait_condition.wait(lock, [&]{ return !!surface; });
    }

    void wait_for_surface_release()
    {
        std::unique_lock<std::mutex> lock(guard);
        wait_condition.wait(lock, [&]{ return !surface; });
    }

    std::mutex guard;
    std::condition_variable wait_condition;
    MirSurface * surface{nullptr};
};

struct ClientSurfaces : mtf::BasicClientServerFixture<mtf::StubbedServerConfiguration>
{
    static const int max_surface_count = 5;
    SurfaceSync ssync[max_surface_count];
};

extern "C" void create_surface_callback(MirSurface* surface, void * context)
{
    SurfaceSync* config = reinterpret_cast<SurfaceSync*>(context);
    config->surface_created(surface);
}

extern "C" void release_surface_callback(MirSurface* surface, void * context)
{
    SurfaceSync* config = reinterpret_cast<SurfaceSync*>(context);
    config->surface_released(surface);
}

void wait_for_surface_create(SurfaceSync* context)
{
    context->wait_for_surface_create();
}

void wait_for_surface_release(SurfaceSync* context)
{
    context->wait_for_surface_release();
}
}

TEST_F(ClientSurfaces, are_created_with_correct_size)
{
    int width_1 = 640, height_1 = 480, width_2 = 1600, height_2 = 1200;
    
    auto spec = mir_connection_create_spec_for_normal_surface(connection, width_1, height_1, mir_pixel_format_abgr_8888);
    mir_surface_create(spec, create_surface_callback, ssync);
    wait_for_surface_create(ssync);

    mir_surface_spec_set_width(spec, width_2);
    mir_surface_spec_set_height(spec, height_2);

    mir_surface_create(spec, create_surface_callback, ssync+1);
    wait_for_surface_create(ssync+1);
    
    mir_surface_spec_release(spec);

    MirSurfaceParameters response_params;
    mir_surface_get_parameters(ssync->surface, &response_params);
    EXPECT_EQ(640, response_params.width);
    EXPECT_EQ(480, response_params.height);
    EXPECT_EQ(mir_pixel_format_abgr_8888, response_params.pixel_format);
    EXPECT_EQ(mir_buffer_usage_hardware, response_params.buffer_usage);

    mir_surface_get_parameters(ssync[1].surface, &response_params);
    EXPECT_EQ(1600, response_params.width);
    EXPECT_EQ(1200, response_params.height);
    EXPECT_EQ(mir_pixel_format_abgr_8888, response_params.pixel_format);
    EXPECT_EQ(mir_buffer_usage_hardware, response_params.buffer_usage);

    mir_surface_release(ssync[1].surface, release_surface_callback, ssync+1);
    wait_for_surface_release(ssync+1);

    mir_surface_release(ssync->surface, release_surface_callback, ssync);
    wait_for_surface_release(ssync);
}

TEST_F(ClientSurfaces, have_distinct_ids)
{
    auto surface_1 = mtf::make_any_surface(connection);
    auto surface_2 = mtf::make_any_surface(connection);
    
    EXPECT_NE(mir_debug_surface_id(surface_1),
        mir_debug_surface_id(surface_2));

    mir_surface_release_sync(surface_1);
    mir_surface_release_sync(surface_2);
}

TEST_F(ClientSurfaces, creates_need_not_be_serialized)
{
    for (int i = 0; i != max_surface_count; ++i)
    {
        auto spec = mir_connection_create_spec_for_normal_surface(connection, 1, 1, mir_pixel_format_abgr_8888);
        mir_surface_create(spec, create_surface_callback, ssync+i);
        mir_surface_spec_release(spec);
    }

    for (int i = 0; i != max_surface_count; ++i)
        wait_for_surface_create(ssync+i);

    for (int i = 0; i != max_surface_count; ++i)
    {
        for (int j = 0; j != max_surface_count; ++j)
        {
            if (i == j)
                EXPECT_EQ(
                    mir_debug_surface_id(ssync[i].surface),
                    mir_debug_surface_id(ssync[j].surface));
            else
                EXPECT_NE(
                    mir_debug_surface_id(ssync[i].surface),
                    mir_debug_surface_id(ssync[j].surface));
        }
    }

    for (int i = 0; i != max_surface_count; ++i)
        mir_surface_release(ssync[i].surface, release_surface_callback, ssync+i);

    for (int i = 0; i != max_surface_count; ++i)
        wait_for_surface_release(ssync+i);
}

