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

#include <cerrno>
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

auto constexpr client_completion_timeout = std::chrono::seconds{6};

namespace mir::wayland
{
extern struct wl_interface const wl_compositor_interface_data;
extern struct wl_interface const wl_surface_interface_data;
extern struct wl_interface const wl_subcompositor_interface_data;
extern struct wl_interface const wl_subsurface_interface_data;
extern struct wl_interface const mir_shell_v1_interface_data;
extern struct wl_interface const mir_positioner_v1_interface_data;
extern struct wl_interface const mir_regular_surface_v1_interface_data;
extern struct wl_interface const mir_menu_surface_v1_interface_data;
}

namespace
{
/// Request opcodes, from the wayland and mir-shell-unstable-v1 protocol definitions.
enum : uint32_t
{
    wl_compositor_create_surface = 0,
    wl_subcompositor_get_subsurface = 1,
    mir_shell_v1_get_regular_surface = 0,
    mir_shell_v1_create_positioner = 4,
    mir_shell_v1_get_menu_surface = 6,
    mir_positioner_v1_set_size = 1,
    mir_positioner_v1_set_anchor_rect = 2,
    mir_menu_surface_v1_reposition = 0,
};

/// Error codes, from the same protocol definitions.
enum : uint32_t
{
    mir_shell_v1_error_invalid_archetype = 0,
    mir_shell_v1_error_invalid_positioner = 1,
    mir_menu_surface_v1_error_invalid_archetype = 0,
    wl_subcompositor_error_bad_surface = 0,
};

struct ProtocolError
{
    std::string interface;
    uint32_t code;

    auto operator==(ProtocolError const&) const -> bool = default;
};

auto operator<<(std::ostream& out, ProtocolError const& error) -> std::ostream&
{
    return out << error.interface << " error " << error.code;
}

/// Adapts a callable into the pair of callbacks miral::InternalClientLauncher expects.
class WaylandClient
{
public:
    void operator()(struct wl_display* display) { code(display); }

    void operator()(std::weak_ptr<mir::scene::Session> const&) {}

    std::function<void(struct wl_display*)> code{[](auto) {}};
};

/// Binds the globals the archetype tests need, runs a scenario against them, and reports any
/// protocol error the server raised in response.
struct MirShellClient
{
    std::function<void(MirShellClient&)> scenario{[](auto&) {}};
    std::optional<ProtocolError> error;

    void operator()(wl_display* display_)
    {
        display = display_;

        auto* const registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &registry_listener, this);
        wl_display_roundtrip(display);

        ASSERT_THAT(compositor, NotNull());
        ASSERT_THAT(subcompositor, NotNull());
        ASSERT_THAT(mir_shell, NotNull());

        scenario(*this);

        // This client runs inside the server, so it must never block waiting for events.
        if (wl_display_roundtrip(display) < 0 && wl_display_get_error(display) == EPROTO)
        {
            wl_interface const* interface{nullptr};
            uint32_t id{0};
            auto const code = wl_display_get_protocol_error(display, &interface, &id);
            error = ProtocolError{interface ? interface->name : "", code};
        }
    }

    auto create_surface() -> wl_proxy*
    {
        return wl_proxy_marshal_constructor(
            compositor, wl_compositor_create_surface, &mir::wayland::wl_surface_interface_data, nullptr);
    }

    auto create_subsurface(wl_proxy* surface, wl_proxy* parent) -> wl_proxy*
    {
        return wl_proxy_marshal_constructor(
            subcompositor,
            wl_subcompositor_get_subsurface,
            &mir::wayland::wl_subsurface_interface_data,
            nullptr,
            surface,
            parent);
    }

    /// A positioner is only complete once both the size and the anchor rect have been set.
    auto create_positioner(bool complete) -> wl_proxy*
    {
        auto* const positioner = wl_proxy_marshal_constructor(
            mir_shell, mir_shell_v1_create_positioner, &mir::wayland::mir_positioner_v1_interface_data, nullptr);

        if (complete)
            wl_proxy_marshal(positioner, mir_positioner_v1_set_size, 100, 200);

        wl_proxy_marshal(positioner, mir_positioner_v1_set_anchor_rect, 0, 0, 400, 400);

        return positioner;
    }

    auto get_regular_surface(wl_proxy* surface) -> wl_proxy*
    {
        return wl_proxy_marshal_constructor(
            mir_shell,
            mir_shell_v1_get_regular_surface,
            &mir::wayland::mir_regular_surface_v1_interface_data,
            nullptr,
            surface);
    }

    auto get_menu_surface(wl_proxy* surface, wl_proxy* positioner) -> wl_proxy*
    {
        return wl_proxy_marshal_constructor(
            mir_shell,
            mir_shell_v1_get_menu_surface,
            &mir::wayland::mir_menu_surface_v1_interface_data,
            nullptr,
            surface,
            positioner);
    }

    static void new_global(void* data, wl_registry* registry, uint32_t id, char const* interface, uint32_t version)
    {
        auto* const self = static_cast<MirShellClient*>(data);

        auto bind = [&](wl_interface const& iface, wl_proxy*& out)
        {
            if (std::string_view(interface) == std::string_view(iface.name))
                out = static_cast<wl_proxy*>(wl_registry_bind(registry, id, &iface, version));
        };

        bind(mir::wayland::wl_compositor_interface_data, self->compositor);
        bind(mir::wayland::wl_subcompositor_interface_data, self->subcompositor);
        bind(mir::wayland::mir_shell_v1_interface_data, self->mir_shell);
    }

    static void global_remove(void*, wl_registry*, uint32_t) {}

    static wl_registry_listener constexpr registry_listener{&new_global, &global_remove};

    wl_display* display{nullptr};
    wl_proxy* compositor{nullptr};
    wl_proxy* subcompositor{nullptr};
    wl_proxy* mir_shell{nullptr};
};

wl_registry_listener constexpr MirShellClient::registry_listener;

struct MirShell : miral::TestServer
{
    MirShell()
    {
        start_server_in_setup = false;
        add_server_init(launcher);
    }

    /// Runs the scenario in an internal client and returns the protocol error it provoked, if any.
    auto run(std::function<void(MirShellClient&)>&& scenario) -> std::optional<ProtocolError>
    {
        mir::test::Signal done;
        std::optional<ProtocolError> error;
        MirShellClient mir_shell_client;
        mir_shell_client.scenario = std::move(scenario);

        client.code = [&](wl_display* display)
        {
            mir_shell_client(display);
            error = mir_shell_client.error;
            done.raise();
        };

        launcher.launch(client);
        EXPECT_TRUE(done.wait_for(client_completion_timeout));

        return error;
    }

private:
    miral::InternalClientLauncher launcher;
    WaylandClient client;
};
}

TEST_F(MirShell, archetype_with_a_complete_positioner_is_accepted)
{
    start_server();

    EXPECT_THAT(
        run(
            [](MirShellClient& client)
            {
                client.get_menu_surface(client.create_surface(), client.create_positioner(true));
            }),
        Eq(std::nullopt));
}

// "If the positioner is incomplete, an invalid_positioner error is raised."
TEST_F(MirShell, archetype_with_an_incomplete_positioner_is_rejected)
{
    start_server();

    EXPECT_THAT(
        run(
            [](MirShellClient& client)
            {
                client.get_menu_surface(client.create_surface(), client.create_positioner(false));
            }),
        Optional(Eq(ProtocolError{"mir_shell_v1", mir_shell_v1_error_invalid_positioner})));
}

// "Assigning an archetype to a surface that already has such a role raises an invalid_archetype
// error on this object."
TEST_F(MirShell, archetype_on_a_subsurface_is_rejected)
{
    start_server();

    EXPECT_THAT(
        run(
            [](MirShellClient& client)
            {
                auto* const parent = client.create_surface();
                auto* const surface = client.create_surface();
                client.create_subsurface(surface, parent);
                client.get_regular_surface(surface);
            }),
        Optional(Eq(ProtocolError{"mir_shell_v1", mir_shell_v1_error_invalid_archetype})));
}

// "Assigning such a role to a surface that already has an archetype raises the error defined by
// the request that assigns the role."
TEST_F(MirShell, subsurface_role_for_a_surface_with_an_archetype_is_rejected)
{
    start_server();

    EXPECT_THAT(
        run(
            [](MirShellClient& client)
            {
                auto* const parent = client.create_surface();
                auto* const surface = client.create_surface();
                client.get_regular_surface(surface);
                client.create_subsurface(surface, parent);
            }),
        Optional(Eq(ProtocolError{"wl_subcompositor", wl_subcompositor_error_bad_surface})));
}

// "a client may assign a new archetype to a surface which already has one"
TEST_F(MirShell, an_archetype_may_be_replaced)
{
    start_server();

    EXPECT_THAT(
        run(
            [](MirShellClient& client)
            {
                auto* const surface = client.create_surface();
                client.get_menu_surface(surface, client.create_positioner(true));
                client.get_regular_surface(surface);
            }),
        Eq(std::nullopt));
}

// "The previous archetype's object becomes inert ... Any further requests made on the previous
// archetype object raise an invalid_archetype error."
TEST_F(MirShell, requests_on_a_replaced_archetype_are_rejected)
{
    start_server();

    EXPECT_THAT(
        run(
            [](MirShellClient& client)
            {
                auto* const surface = client.create_surface();
                auto* const positioner = client.create_positioner(true);
                auto* const menu = client.get_menu_surface(surface, positioner);
                client.get_regular_surface(surface);
                wl_proxy_marshal(menu, mir_menu_surface_v1_reposition, positioner, 1);
            }),
        Optional(Eq(ProtocolError{"mir_menu_surface_v1", mir_menu_surface_v1_error_invalid_archetype})));
}
