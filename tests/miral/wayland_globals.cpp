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

#include <miral/test_server.h>

#include <miral/internal_client.h>

#include <mir/test/signal.h>

#include <wayland-client.h>

#include <chrono>
#include <set>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace
{
auto constexpr client_completion_timeout = std::chrono::seconds{6};

struct WaylandGlobals : miral::TestServer
{
    WaylandGlobals()
    {
        // Use the stub rendering platform, whose buffer allocator offers no linux-dmabuf support,
        // rather than whatever native rendering platform the host happens to support.
        add_to_environment("MIR_SERVER_PLATFORM_RENDERING_LIBS", "mir:stub-graphics");
        start_server_in_setup = false;
        add_server_init(launcher);
    }

    /// The interface names of the globals advertised to an internal client
    auto advertised_globals() -> std::set<std::string>
    {
        mir::test::Signal done;
        std::set<std::string> globals;

        launcher.launch(
            [&](wl_display* display)
            {
                auto* const registry = wl_display_get_registry(display);
                wl_registry_add_listener(registry, &registry_listener, &globals);
                wl_display_roundtrip(display);
                wl_registry_destroy(registry);
                done.raise();
            },
            [](auto) {});

        EXPECT_TRUE(done.wait_for(client_completion_timeout));
        return globals;
    }

private:
    static void new_global(void* data, wl_registry*, uint32_t, char const* interface, uint32_t)
    {
        static_cast<std::set<std::string>*>(data)->insert(interface);
    }

    static void global_remove(void*, wl_registry*, uint32_t) {}

    static wl_registry_listener constexpr registry_listener{&new_global, &global_remove};

    miral::InternalClientLauncher launcher;
};
}

// The graphics platform decides which client buffer types it supports, so the global must not
// be advertised when the platform does not offer linux-dmabuf support.
TEST_F(WaylandGlobals, linux_dmabuf_is_not_advertised_when_the_platform_does_not_offer_it)
{
    start_server();

    auto const globals = advertised_globals();

    EXPECT_THAT(globals, Contains("wl_compositor"));
    EXPECT_THAT(globals, Not(Contains("zwp_linux_dmabuf_v1")));
}
