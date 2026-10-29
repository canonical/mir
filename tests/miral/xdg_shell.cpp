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

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <functional>
#include <optional>
#include <ostream>
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
extern struct wl_interface const xdg_wm_base_interface_data;
extern struct wl_interface const xdg_surface_interface_data;
extern struct wl_interface const xdg_toplevel_interface_data;
extern struct wl_interface const xdg_positioner_interface_data;
extern struct wl_interface const xdg_popup_interface_data;
}

namespace
{
/// Request opcodes, from the xdg-shell protocol definition.
enum : uint32_t
{
    wl_compositor_create_surface = 0,
    xdg_wm_base_create_positioner = 1,
    xdg_wm_base_get_xdg_surface = 2,
    xdg_surface_get_toplevel = 1,
    xdg_surface_get_popup = 2,
    xdg_positioner_set_size = 1,
    xdg_positioner_set_anchor_rect = 2,
    xdg_popup_reposition = 2,
};

/// Error codes, from the same protocol definition. Note that `invalid_positioner` belongs to
/// xdg_wm_base: a client decodes an error code against the interface of the object it arrives
/// on, so the interface these tests assert matters as much as the code.
enum : uint32_t
{
    xdg_wm_base_error_invalid_positioner = 5,
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

/// Binds the globals the positioner tests need, runs a scenario against them, and reports any
/// protocol error the server raised in response.
struct XdgShellClient
{
    std::function<void(XdgShellClient&)> scenario{[](auto&) {}};
    std::optional<ProtocolError> error;

    void operator()(wl_display* display_)
    {
        display = display_;

        auto* const registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &registry_listener, this);
        wl_display_roundtrip(display);

        ASSERT_THAT(compositor, NotNull());
        ASSERT_THAT(wm_base, NotNull());

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

    auto get_xdg_surface(wl_proxy* surface) -> wl_proxy*
    {
        return wl_proxy_marshal_constructor(
            wm_base, xdg_wm_base_get_xdg_surface, &mir::wayland::xdg_surface_interface_data, nullptr, surface);
    }

    /// A popup needs a parent, and only a surface with a role can be one.
    auto create_toplevel() -> wl_proxy*
    {
        auto* const xdg_surface = get_xdg_surface(create_surface());
        wl_proxy_marshal_constructor(
            xdg_surface, xdg_surface_get_toplevel, &mir::wayland::xdg_toplevel_interface_data, nullptr);
        return xdg_surface;
    }

    /// A positioner is only complete once both the size and the anchor rect have been set.
    auto create_positioner(bool complete) -> wl_proxy*
    {
        auto* const positioner = wl_proxy_marshal_constructor(
            wm_base, xdg_wm_base_create_positioner, &mir::wayland::xdg_positioner_interface_data, nullptr);

        if (complete)
            wl_proxy_marshal(positioner, xdg_positioner_set_size, 100, 200);

        wl_proxy_marshal(positioner, xdg_positioner_set_anchor_rect, 0, 0, 400, 400);

        return positioner;
    }

    auto get_popup(wl_proxy* xdg_surface, wl_proxy* parent, wl_proxy* positioner) -> wl_proxy*
    {
        return wl_proxy_marshal_constructor(
            xdg_surface,
            xdg_surface_get_popup,
            &mir::wayland::xdg_popup_interface_data,
            nullptr,
            parent,
            positioner);
    }

    void reposition(wl_proxy* popup, wl_proxy* positioner)
    {
        wl_proxy_marshal(popup, xdg_popup_reposition, positioner, 1);
    }

    static void new_global(void* data, wl_registry* registry, uint32_t id, char const* interface, uint32_t version)
    {
        auto* const self = static_cast<XdgShellClient*>(data);

        auto bind = [&](wl_interface const& iface, wl_proxy*& out)
        {
            if (std::string_view(interface) == std::string_view(iface.name))
                out = static_cast<wl_proxy*>(
                    wl_registry_bind(registry, id, &iface, std::min(version, static_cast<uint32_t>(iface.version))));
        };

        bind(mir::wayland::wl_compositor_interface_data, self->compositor);
        bind(mir::wayland::xdg_wm_base_interface_data, self->wm_base);
    }

    static void global_remove(void*, wl_registry*, uint32_t) {}

    static wl_registry_listener constexpr registry_listener{&new_global, &global_remove};

    wl_display* display{nullptr};
    wl_proxy* compositor{nullptr};
    wl_proxy* wm_base{nullptr};
};

wl_registry_listener constexpr XdgShellClient::registry_listener;

struct XdgShell : miral::TestServer
{
    XdgShell()
    {
        start_server_in_setup = false;
        add_server_init(launcher);
    }

    /// Runs the scenario in an internal client and returns the protocol error it provoked, if any.
    auto run(std::function<void(XdgShellClient&)>&& scenario) -> std::optional<ProtocolError>
    {
        mir::test::Signal done;
        std::optional<ProtocolError> error;
        XdgShellClient xdg_shell_client;
        xdg_shell_client.scenario = std::move(scenario);

        client.code = [&](wl_display* display)
        {
            xdg_shell_client(display);
            error = xdg_shell_client.error;
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

// "Passing an incomplete xdg_positioner object when positioning a surface raises an
// invalid_positioner error." That error is defined by xdg_wm_base, so it has to arrive on the
// xdg_wm_base: sent on the xdg_positioner instead, a client would decode code 5 as whatever
// xdg_positioner happens to define, which is nothing.
TEST_F(XdgShell, get_popup_with_an_incomplete_positioner_is_rejected_on_the_wm_base)
{
    start_server();

    EXPECT_THAT(
        run(
            [](XdgShellClient& client)
            {
                auto* const parent = client.create_toplevel();
                client.get_popup(
                    client.get_xdg_surface(client.create_surface()), parent, client.create_positioner(false));
            }),
        Optional(Eq(ProtocolError{"xdg_wm_base", xdg_wm_base_error_invalid_positioner})));
}

TEST_F(XdgShell, reposition_with_an_incomplete_positioner_is_rejected_on_the_wm_base)
{
    start_server();

    EXPECT_THAT(
        run(
            [](XdgShellClient& client)
            {
                auto* const parent = client.create_toplevel();
                auto* const popup = client.get_popup(
                    client.get_xdg_surface(client.create_surface()), parent, client.create_positioner(true));
                client.reposition(popup, client.create_positioner(false));
            }),
        Optional(Eq(ProtocolError{"xdg_wm_base", xdg_wm_base_error_invalid_positioner})));
}

TEST_F(XdgShell, get_popup_with_a_complete_positioner_is_accepted)
{
    start_server();

    EXPECT_THAT(
        run(
            [](XdgShellClient& client)
            {
                auto* const parent = client.create_toplevel();
                client.get_popup(
                    client.get_xdg_surface(client.create_surface()), parent, client.create_positioner(true));
            }),
        Eq(std::nullopt));
}

TEST_F(XdgShell, reposition_with_a_complete_positioner_is_accepted)
{
    start_server();

    EXPECT_THAT(
        run(
            [](XdgShellClient& client)
            {
                auto* const parent = client.create_toplevel();
                auto* const popup = client.get_popup(
                    client.get_xdg_surface(client.create_surface()), parent, client.create_positioner(true));
                client.reposition(popup, client.create_positioner(true));
            }),
        Eq(std::nullopt));
}
