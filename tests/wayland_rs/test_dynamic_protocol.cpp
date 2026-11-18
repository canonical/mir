/*
 * Copyright © Canonical Ltd.
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
 */

#include "wayland_rs_server_test.h"

#include "mir_test_dynamic_v1_wrapper.h"
#include "protocol_error.h"

#include <mir/synchronised.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <wayland-client.h>

#include <sys/socket.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <format>
#include <future>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

namespace mrs = mir::wayland;
namespace generated = mir::wayland::mir_test_dynamic_v1;
using namespace std::chrono_literals;
using testing::ElementsAre;
using testing::HasSubstr;

namespace
{
char const* const manager_name = generated::TestDynamicManagerV1::interface_name;
char const* const child_name = generated::TestDynamicChildV1::interface_name;

// Client-side opcodes. The server side uses the generated wrapper
// (mir_test_dynamic_v1.xml).
namespace opcode
{
uint32_t const destroy = 0;
uint32_t const ping = 1;
uint32_t const create_child = 2;
uint32_t const trigger_error = 3;
uint32_t const adopt = 4;
uint32_t const pong = 0;
uint32_t const child_created = 1;
uint32_t const echo_fixed = 1;
uint32_t const value = 0;
uint32_t const fixed_value = 1;
}

// libwayland-client descriptions of the test protocol, as wayland-scanner would generate.
extern wl_interface const child_interface;

wl_interface const* no_types[] = {nullptr, nullptr};
wl_interface const* create_child_types[] = {&child_interface, nullptr};
wl_interface const* child_types[] = {&child_interface};

wl_message const manager_requests[] = {
    {"destroy", "", no_types},
    {"ping", "us", no_types},
    {"create_child", "ni", create_child_types},
    {"trigger_error", "u", no_types},
    {"adopt", "?o", child_types},
};
wl_message const manager_events[] = {
    {"pong", "us", no_types},
    {"child_created", "2n", child_types},
};
wl_interface const manager_interface = {manager_name, 2, 5, manager_requests, 2, manager_events};

wl_message const child_requests[] = {
    {"destroy", "", no_types},
    {"echo_fixed", "f", no_types},
};
wl_message const child_events[] = {
    {"value", "i", no_types},
    {"fixed_value", "f", no_types},
};
wl_interface const child_interface = {child_name, 2, 2, child_requests, 2, child_events};

auto uint_arg(uint32_t value) -> mrs::DynamicArgument
{
    mrs::DynamicArgument arg{};
    arg.kind = mrs::DynamicArgumentKind::Uint;
    arg.uint_value = value;
    return arg;
}

auto int_arg(int32_t value) -> mrs::DynamicArgument
{
    mrs::DynamicArgument arg{};
    arg.kind = mrs::DynamicArgumentKind::Int;
    arg.int_value = value;
    return arg;
}

auto string_arg(std::string const& value) -> mrs::DynamicArgument
{
    mrs::DynamicArgument arg{};
    arg.kind = mrs::DynamicArgumentKind::String;
    arg.string_value = value;
    return arg;
}

auto null_string_arg() -> mrs::DynamicArgument
{
    mrs::DynamicArgument arg{};
    arg.kind = mrs::DynamicArgumentKind::String;
    arg.is_null = true;
    return arg;
}

auto new_id_arg(uint32_t object_id) -> mrs::DynamicArgument
{
    mrs::DynamicArgument arg{};
    arg.kind = mrs::DynamicArgumentKind::NewId;
    arg.object_id = object_id;
    return arg;
}

void send(mrs::DynamicResource const& resource, uint32_t opcode, std::vector<mrs::DynamicArgument> const& args)
{
    resource.send_event(opcode, rust::Slice<mrs::DynamicArgument const>{args.data(), args.size()});
}

/// What the server-side handlers observed, in order. Written on the event-loop
/// thread, read on the test thread.
class Log
{
public:
    void add(std::string entry)
    {
        entries.lock()->push_back(std::move(entry));
        changed.notify_all();
    }

    auto snapshot() -> std::vector<std::string> { return *entries.lock(); }

    auto wait_for(std::string const& entry) -> bool
    {
        auto locked = entries.lock();
        return locked.wait_for(changed, 5s, [&] { return std::ranges::find(*locked, entry) != locked->end(); });
    }

private:
    mir::Synchronised<std::vector<std::string>> entries;
    std::condition_variable changed;
};

class Child : public generated::TestDynamicChildV1
{
public:
    Child(rust::Box<mrs::DynamicResource> resource, Log& log) : TestDynamicChildV1{std::move(resource)}, log{log} {}

    void on_destroyed() override { log.add("child.destroyed"); }

private:
    void destroy() override { log.add("child.destroy"); }

    void echo_fixed(double value) override
    {
        log.add(std::format("child.echo_fixed {}", value));
        send_fixed_value_event(value);
    }

    Log& log;
};

class Manager : public generated::TestDynamicManagerV1
{
public:
    Manager(rust::Box<mrs::DynamicResource> resource, Log& log) : TestDynamicManagerV1{std::move(resource)}, log{log}
    {
    }

    void on_destroyed() override { log.add("manager.destroyed"); }

private:
    void destroy() override { log.add("manager.destroy"); }

    void ping(uint32_t serial, std::string const& message) override
    {
        log.add(std::format("manager.ping {} {}", serial, message));
        send_pong_event(serial, message);
    }

    auto create_child(rust::Box<mrs::DynamicResource> id, int32_t value)
        -> std::shared_ptr<generated::TestDynamicChildV1> override
    {
        log.add(std::format("manager.create_child {} v{}", std::string{id->interface_name()}, id->version()));
        auto child = std::make_shared<Child>(std::move(id), log);
        child->send_value_event(value);
        return child;
    }

    void trigger_error(uint32_t code) override
    {
        if (code == 0)
            throw std::runtime_error{"not a protocol error"};
        throw mrs::ProtocolError{object_id(), code, "triggered"};
    }

    void adopt(std::shared_ptr<generated::TestDynamicChildV1> const& child) override
    {
        log.add(
            "manager.adopt " +
            (child ? std::to_string(child->object_id()) + (std::dynamic_pointer_cast<Child>(child) ? " Child" : " ?")
                   : std::string{"null"}));
    }

    Log& log;
};

class Global : public generated::TestDynamicManagerV1::Global
{
public:
    explicit Global(Log& log) : log{log} {}

    /// The bound managers. Only touch on the event-loop thread.
    std::vector<std::weak_ptr<Manager>> managers;

private:
    auto on_bind(rust::Box<mrs::WaylandClient>, rust::Box<mrs::DynamicResource> resource)
        -> std::shared_ptr<generated::TestDynamicManagerV1> override
    {
        log.add("bind v" + std::to_string(resource->version()));
        auto manager = std::make_shared<Manager>(std::move(resource), log);
        managers.push_back(manager);
        return manager;
    }

    Log& log;
};

/// What the client observed.
struct ClientState
{
    std::optional<uint32_t> global_name;
    std::vector<std::string> events;
    wl_proxy* server_child{nullptr};
};

void registry_global(void* data, wl_registry*, uint32_t name, char const* interface, uint32_t)
{
    if (std::string{interface} == manager_name)
        static_cast<ClientState*>(data)->global_name = name;
}

void registry_global_remove(void*, wl_registry*, uint32_t) {}

wl_registry_listener const registry_listener{registry_global, registry_global_remove};

void manager_pong(void* data, wl_proxy*, uint32_t serial, char const* message)
{
    static_cast<ClientState*>(data)->events.push_back("pong " + std::to_string(serial) + " " + message);
}

void manager_child_created(void* data, wl_proxy*, wl_proxy* child)
{
    auto* const state = static_cast<ClientState*>(data);
    state->events.push_back("child_created");
    state->server_child = child;
}

struct ManagerListener
{
    decltype(&manager_pong) pong;
    decltype(&manager_child_created) child_created;
} const manager_listener{manager_pong, manager_child_created};

void child_value(void* data, wl_proxy*, int32_t value)
{
    static_cast<ClientState*>(data)->events.push_back("value " + std::to_string(value));
}

void child_fixed_value(void* data, wl_proxy*, wl_fixed_t value)
{
    static_cast<ClientState*>(data)->events.push_back("fixed_value " + std::to_string(value));
}

struct ChildListener
{
    decltype(&child_value) value;
    decltype(&child_fixed_value) fixed_value;
} const child_listener{child_value, child_fixed_value};

class DynamicProtocolTest : public mrs::test::RunningWaylandServerTest
{
public:
    void SetUp() override
    {
        generated::register_protocol();
        RunningWaylandServerTest::SetUp();
    }

    void TearDown() override
    {
        if (display)
            wl_display_disconnect(display);
        if (global)
            on_loop([this] { global.reset(); });

        RunningWaylandServerTest::TearDown();
    }

    /// Run `work` on the server's event loop and return its result.
    template<typename Work>
    auto on_loop(Work work) -> decltype(work())
    {
        auto task = std::make_shared<std::packaged_task<decltype(work())()>>(std::move(work));
        auto result = task->get_future();
        executor->spawn([task] { (*task)(); });
        if (result.wait_for(5s) != std::future_status::ready)
            throw std::runtime_error{"Timed out waiting for the server event loop"};
        return result.get();
    }

    void create_global(uint32_t version)
    {
        auto handler = std::make_unique<Global>(log);
        global_handler = handler.get();
        global.emplace(on_loop([&]
            { return generated::TestDynamicManagerV1::Global::create(**server, std::move(handler), version); }));
    }

    void connect_client()
    {
        int fds[2]{-1, -1};
        ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds), 0);
        display = wl_display_connect_to_fd(fds[1]);
        if (!display)
        {
            ::close(fds[0]);
            FAIL() << "Failed to connect Wayland client";
        }
        (*server)->insert_client(fds[0]);
    }

    /// Connect a client and bind the manager global at `version`.
    auto bind_manager(uint32_t version) -> wl_proxy*
    {
        connect_client();
        if (!display)
            return nullptr;
        auto* const registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &registry_listener, &client);
        wl_display_roundtrip(display);
        if (!client.global_name)
            return nullptr;

        auto* const manager =
            static_cast<wl_proxy*>(wl_registry_bind(registry, *client.global_name, &manager_interface, version));
        wl_proxy_add_listener(
            manager, reinterpret_cast<void (**)()>(const_cast<ManagerListener*>(&manager_listener)), &client);
        wl_display_roundtrip(display);
        return manager;
    }

    void roundtrip()
    {
        ASSERT_NE(wl_display_roundtrip(display), -1) << "Unexpected protocol error " << wl_display_get_error(display);
    }

    /// The first bound manager. Only call on the event-loop thread.
    auto first_manager() -> std::shared_ptr<Manager> { return global_handler->managers.at(0).lock(); }

    Log log;
    Global* global_handler{nullptr};
    std::optional<rust::Box<mrs::DynamicGlobal>> global;
    wl_display* display{nullptr};
    ClientState client;
};

template<typename T>
auto to_rust(std::vector<T> items) -> rust::Vec<T>
{
    rust::Vec<T> result;
    for (auto& item : items)
        result.push_back(std::move(item));
    return result;
}

auto argument(std::string const& name, mrs::DynamicArgumentKind kind, std::string const& interface = "")
    -> mrs::DynamicArgumentDescriptor
{
    return {rust::String{name}, kind, false, rust::String{interface}};
}

auto message(std::string const& name, std::vector<mrs::DynamicArgumentDescriptor> args = {})
    -> mrs::DynamicMessageDescriptor
{
    return {rust::String{name}, 1, false, to_rust(std::move(args))};
}

auto interface(
    std::string const& name,
    std::vector<mrs::DynamicMessageDescriptor> requests = {},
    std::vector<mrs::DynamicMessageDescriptor> events = {}) -> mrs::DynamicInterfaceDescriptor
{
    return {rust::String{name}, 1, to_rust(std::move(requests)), to_rust(std::move(events))};
}

auto protocol(std::string const& name, std::vector<mrs::DynamicInterfaceDescriptor> interfaces)
    -> mrs::DynamicProtocolDescriptor
{
    return {rust::String{name}, to_rust(std::move(interfaces))};
}

auto expect_registration_error(mrs::DynamicProtocolDescriptor const& protocol) -> std::string
{
    try
    {
        mrs::register_dynamic_protocol(protocol);
    }
    catch (rust::Error const& error)
    {
        return error.what();
    }
    ADD_FAILURE() << "Registering the protocol unexpectedly succeeded";
    return {};
}
}

TEST(DynamicProtocolRegistration, registering_identical_protocol_again_succeeds)
{
    generated::register_protocol();
    EXPECT_NO_THROW(generated::register_protocol());
}

TEST(DynamicProtocolRegistration, rejects_different_protocol_with_same_name)
{
    generated::register_protocol();
    auto const error = expect_registration_error(protocol("mir_test_dynamic_v1", {interface(manager_name)}));
    EXPECT_THAT(error, HasSubstr("mir_test_dynamic_v1"));
}

TEST(DynamicProtocolRegistration, rejects_inconsistent_descriptors)
{
    auto const rejected = [](mrs::DynamicInterfaceDescriptor interface)
    {
        return !expect_registration_error(protocol("mir_test_inconsistent", {std::move(interface)})).empty();
    };

    auto unnamed = interface("");
    EXPECT_TRUE(rejected(unnamed)) << "unnamed interface";

    auto version_0 = interface("mir_test_inconsistent_v1");
    version_0.version = 0;
    EXPECT_TRUE(rejected(version_0)) << "version 0";

    auto too_new = interface("mir_test_inconsistent_v1", {message("request")});
    too_new.requests[0].since = 2;
    EXPECT_TRUE(rejected(too_new)) << "message newer than its interface";

    auto nullable_uint =
        interface("mir_test_inconsistent_v1", {message("request", {argument("a", mrs::DynamicArgumentKind::Uint)})});
    nullable_uint.requests[0].args[0].allow_null = true;
    EXPECT_TRUE(rejected(nullable_uint)) << "nullable uint";

    EXPECT_TRUE(rejected(interface(
        "mir_test_inconsistent_v1",
        {message("request", {argument("a", mrs::DynamicArgumentKind::Int, "wl_surface")})})))
        << "int with an interface";

    EXPECT_TRUE(rejected(interface(
        "mir_test_inconsistent_v1",
        {message("request", {argument("a", static_cast<mrs::DynamicArgumentKind>(200))})})))
        << "unknown kind";
}

TEST(DynamicProtocolRegistration, rejects_redefining_builtin_interface)
{
    auto const error = expect_registration_error(protocol("mir_test_clash_builtin", {interface("wl_compositor")}));
    EXPECT_THAT(error, HasSubstr("wl_compositor"));
}

TEST(DynamicProtocolRegistration, rejects_redefining_interface_of_another_registered_protocol)
{
    generated::register_protocol();
    auto const error = expect_registration_error(protocol("mir_test_clash_dynamic", {interface(child_name)}));
    EXPECT_THAT(error, HasSubstr(child_name));
}

TEST(DynamicProtocolRegistration, rejects_new_id_without_interface)
{
    auto const error = expect_registration_error(protocol(
        "mir_test_untyped_new_id",
        {interface(
            "mir_test_untyped_new_id_v1", {message("bind", {argument("id", mrs::DynamicArgumentKind::NewId)})})}));
    EXPECT_THAT(error, HasSubstr("new_id"));
}

TEST(DynamicProtocolRegistration, rejects_new_id_of_builtin_interface)
{
    auto const error = expect_registration_error(protocol(
        "mir_test_builtin_new_id",
        {interface(
            "mir_test_builtin_new_id_v1",
            {message("sync", {argument("callback", mrs::DynamicArgumentKind::NewId, "wl_callback")})})}));
    EXPECT_THAT(error, HasSubstr("wl_callback"));
}

TEST(DynamicProtocolRegistration, rejects_new_id_cycle)
{
    auto const error = expect_registration_error(protocol(
        "mir_test_cycle",
        {interface(
             "mir_test_cycle_a_v1",
             {message("make_b", {argument("id", mrs::DynamicArgumentKind::NewId, "mir_test_cycle_b_v1")})}),
         interface(
             "mir_test_cycle_b_v1",
             {message("make_a", {argument("id", mrs::DynamicArgumentKind::NewId, "mir_test_cycle_a_v1")})})}));
    EXPECT_THAT(error, HasSubstr("cycle"));
}

TEST(DynamicProtocolRegistration, failed_registration_registers_nothing)
{
    expect_registration_error(
        protocol("mir_test_partial", {interface("mir_test_partial_ok_v1"), interface("wl_surface")}));

    // Had the valid interface been registered, this would clash with it.
    EXPECT_NO_THROW(
        mrs::register_dynamic_protocol(protocol("mir_test_partial_retry", {interface("mir_test_partial_ok_v1")})));
}

TEST(DynamicProtocolRegistration, creating_global_fails_when_server_is_not_running)
{
    Log log;
    auto server = mrs::create_wayland_server();
    EXPECT_THROW(
        generated::TestDynamicManagerV1::Global::create(*server, std::make_unique<Global>(log)), rust::Error);
}

TEST_F(DynamicProtocolTest, creating_global_rejects_bad_arguments)
{
    auto const try_create = [this](char const* interface, uint32_t version, bool with_handler)
    {
        return on_loop(
            [&]() -> bool
            {
                try
                {
                    auto handler = with_handler ? std::make_unique<Global>(log) : nullptr;
                    (*server)->create_dynamic_global(interface, version, std::move(handler));
                    return true;
                }
                catch (rust::Error const&)
                {
                    return false;
                }
            });
    };

    EXPECT_FALSE(try_create("mir_test_never_registered_v1", 1, true));
    EXPECT_FALSE(try_create(manager_name, 0, true));
    EXPECT_FALSE(try_create(manager_name, 3, true));
    EXPECT_FALSE(try_create(manager_name, 2, false));
    EXPECT_TRUE(try_create(manager_name, 2, true));
}

TEST_F(DynamicProtocolTest, client_can_bind_global)
{
    create_global(2);
    auto* const manager = bind_manager(1);
    ASSERT_TRUE(manager);

    EXPECT_THAT(log.snapshot(), ElementsAre("bind v1"));
}

TEST_F(DynamicProtocolTest, global_can_be_hidden_from_client)
{
    class HidingGlobal : public Global
    {
    public:
        using Global::Global;
        auto can_view(rust::Box<mrs::WaylandClientId>) -> bool override { return false; }
    };
    global.emplace(on_loop(
        [&]
        {
            return generated::TestDynamicManagerV1::Global::create(**server, std::make_unique<HidingGlobal>(log));
        }));

    EXPECT_FALSE(bind_manager(1));
}

TEST_F(DynamicProtocolTest, removing_global_withdraws_it)
{
    create_global(2);
    on_loop([this] { global.reset(); });

    EXPECT_FALSE(bind_manager(1));
}

TEST_F(DynamicProtocolTest, request_is_dispatched_and_event_is_delivered)
{
    create_global(2);
    auto* const manager = bind_manager(2);
    ASSERT_TRUE(manager);

    wl_proxy_marshal_flags(manager, opcode::ping, nullptr, 2, 0, 42u, "hello");
    roundtrip();

    EXPECT_THAT(log.snapshot(), ElementsAre("bind v2", "manager.ping 42 hello"));
    EXPECT_THAT(client.events, ElementsAre("pong 42 hello"));
}

TEST_F(DynamicProtocolTest, constructor_request_creates_object_with_handler)
{
    create_global(2);
    auto* const manager = bind_manager(2);
    ASSERT_TRUE(manager);

    auto* const child = wl_proxy_marshal_flags(manager, opcode::create_child, &child_interface, 2, 0, nullptr, -7);
    wl_proxy_add_listener(child, reinterpret_cast<void (**)()>(const_cast<ChildListener*>(&child_listener)), &client);
    wl_proxy_marshal_flags(child, opcode::echo_fixed, nullptr, 2, 0, wl_fixed_from_double(1.5));
    roundtrip();

    EXPECT_THAT(
        log.snapshot(),
        ElementsAre("bind v2", "manager.create_child mir_test_dynamic_child_v1 v2", "child.echo_fixed 1.5"));
    EXPECT_THAT(client.events, ElementsAre("value -7", "fixed_value " + std::to_string(wl_fixed_from_double(1.5))));
}

TEST_F(DynamicProtocolTest, object_argument_arrives_as_its_handler)
{
    create_global(2);
    auto* const manager = bind_manager(2);
    ASSERT_TRUE(manager);

    auto* const child = wl_proxy_marshal_flags(manager, opcode::create_child, &child_interface, 2, 0, nullptr, 1);
    wl_proxy_marshal_flags(manager, opcode::adopt, nullptr, 2, 0, child);
    wl_proxy_marshal_flags(manager, opcode::adopt, nullptr, 2, 0, nullptr);
    roundtrip();

    EXPECT_THAT(
        log.snapshot(),
        ElementsAre(
            "bind v2",
            "manager.create_child mir_test_dynamic_child_v1 v2",
            "manager.adopt " + std::to_string(wl_proxy_get_id(child)) + " Child",
            "manager.adopt null"));
}

TEST_F(DynamicProtocolTest, server_can_create_object_and_send_it_in_event)
{
    create_global(2);
    auto* const manager = bind_manager(2);
    ASSERT_TRUE(manager);

    on_loop(
        [this]
        {
            auto const manager = first_manager();
            manager->send_child_created_event(
                std::make_shared<Child>(manager->resource().create_resource(child_name), log));
        });
    roundtrip();
    ASSERT_TRUE(client.server_child);

    wl_proxy_add_listener(
        client.server_child, reinterpret_cast<void (**)()>(const_cast<ChildListener*>(&child_listener)), &client);
    wl_proxy_marshal_flags(client.server_child, opcode::echo_fixed, nullptr, 2, 0, wl_fixed_from_int(3));
    roundtrip();

    EXPECT_THAT(client.events, ElementsAre("child_created", "fixed_value " + std::to_string(wl_fixed_from_int(3))));
}

TEST_F(DynamicProtocolTest, handler_rejects_resource_of_another_interface)
{
    create_global(2);
    ASSERT_TRUE(bind_manager(2));

    EXPECT_TRUE(on_loop(
        [this]
        {
            try
            {
                Child{first_manager()->resource().create_resource(manager_name), log};
                return false;
            }
            catch (std::invalid_argument const&)
            {
                return true;
            }
        }));
}

TEST_F(DynamicProtocolTest, destructor_request_notifies_destruction)
{
    create_global(2);
    auto* const manager = bind_manager(2);
    ASSERT_TRUE(manager);

    auto* const child = wl_proxy_marshal_flags(manager, opcode::create_child, &child_interface, 2, 0, nullptr, 1);
    wl_proxy_marshal_flags(child, opcode::destroy, nullptr, 2, WL_MARSHAL_FLAG_DESTROY);
    roundtrip();

    EXPECT_THAT(
        log.snapshot(),
        ElementsAre(
            "bind v2", "manager.create_child mir_test_dynamic_child_v1 v2", "child.destroy", "child.destroyed"));
}

TEST_F(DynamicProtocolTest, client_disconnect_notifies_destruction)
{
    create_global(2);
    auto* const manager = bind_manager(2);
    ASSERT_TRUE(manager);
    wl_proxy_marshal_flags(manager, opcode::create_child, &child_interface, 2, 0, nullptr, 1);
    roundtrip();

    wl_display_disconnect(display);
    display = nullptr;

    EXPECT_TRUE(log.wait_for("manager.destroyed"));
    EXPECT_TRUE(log.wait_for("child.destroyed"));
}

TEST_F(DynamicProtocolTest, protocol_error_from_handler_is_posted_to_client)
{
    create_global(2);
    auto* const manager = bind_manager(2);
    ASSERT_TRUE(manager);

    wl_proxy_marshal_flags(manager, opcode::trigger_error, nullptr, 2, 0, 5u);
    EXPECT_EQ(wl_display_roundtrip(display), -1);

    wl_interface const* interface{nullptr};
    uint32_t id{0};
    EXPECT_EQ(wl_display_get_protocol_error(display, &interface, &id), 5u);
    EXPECT_EQ(interface, &manager_interface);
    EXPECT_EQ(id, wl_proxy_get_id(manager));
}

TEST_F(DynamicProtocolTest, other_exception_from_handler_is_an_implementation_error)
{
    create_global(2);
    auto* const manager = bind_manager(2);
    ASSERT_TRUE(manager);

    wl_proxy_marshal_flags(manager, opcode::trigger_error, nullptr, 2, 0, 0u);
    EXPECT_EQ(wl_display_roundtrip(display), -1);

    wl_interface const* interface{nullptr};
    uint32_t id{0};
    EXPECT_EQ(
        wl_display_get_protocol_error(display, &interface, &id),
        static_cast<uint32_t>(WL_DISPLAY_ERROR_IMPLEMENTATION));
    EXPECT_EQ(interface, &wl_display_interface);
}

TEST_F(DynamicProtocolTest, invalid_events_are_rejected_without_sending)
{
    create_global(2);
    auto* const manager = bind_manager(1);
    ASSERT_TRUE(manager);

    auto const rejected = [this](uint32_t opcode, std::vector<mrs::DynamicArgument> args)
    {
        return on_loop(
            [&]() -> bool
            {
                try
                {
                    send(first_manager()->resource(), opcode, args);
                    return false;
                }
                catch (rust::Error const&)
                {
                    return true;
                }
            });
    };

    EXPECT_TRUE(rejected(7, {})) << "unknown opcode";
    EXPECT_TRUE(rejected(opcode::pong, {uint_arg(1)})) << "too few arguments";
    EXPECT_TRUE(rejected(opcode::pong, {int_arg(1), string_arg("x")})) << "wrong argument type";
    EXPECT_TRUE(rejected(opcode::pong, {uint_arg(1), null_string_arg()})) << "null non-nullable string";
    EXPECT_TRUE(rejected(opcode::pong, {uint_arg(1), string_arg(std::string{"a\0b", 3})})) << "embedded null";
    EXPECT_TRUE(rejected(opcode::child_created, {new_id_arg(1000)})) << "event newer than the object";
    EXPECT_FALSE(on_loop([this] { return first_manager()->version_supports_child_created(); }));

    roundtrip();
    EXPECT_THAT(client.events, testing::IsEmpty());
}
