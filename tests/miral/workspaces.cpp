/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#include <miral/window_management_policy.h>
#include <miral/window_manager_tools.h>

#include <mir/client/connection.h>
#include <mir/client/surface.h>
#include <mir/client/window.h>
#include <mir/client/window_spec.h>
#include <mir_toolkit/mir_buffer_stream.h>

#include "test_server.h"

#include <gmock/gmock.h>
#include <mir/test/signal.h>


using namespace testing;
using namespace mir::client;
using namespace std::chrono_literals;
using miral::WindowManagerTools;

namespace
{
std::string const top_level{"top level"};
std::string const dialog{"dialog"};
std::string const tip{"tip"};
std::string const a_window{"a window"};
std::string const another_window{"another window"};

struct Workspaces;

struct WorkspacesWindowManagerPolicy : miral::TestServer::TestWindowManagerPolicy
{
    WorkspacesWindowManagerPolicy(WindowManagerTools const& tools, Workspaces& test_fixture);
    ~WorkspacesWindowManagerPolicy();

    void advise_new_window(miral::WindowInfo const& window_info);
    void handle_window_ready(miral::WindowInfo& window_info);

    MOCK_METHOD2(advise_adding_to_workspace,
                 void(std::shared_ptr<miral::Workspace> const&, std::vector<miral::Window> const&));

    MOCK_METHOD2(advise_removing_from_workspace,
                 void(std::shared_ptr<miral::Workspace> const&, std::vector<miral::Window> const&));

    MOCK_METHOD1(advise_focus_gained, void(miral::WindowInfo const&));

    MOCK_METHOD1(advise_window_ready, void(miral::WindowInfo const&));

    Workspaces& test_fixture;
};

struct TestWindow : Surface, Window
{
    using Surface::operator=;
    using Window::operator=;
};

struct Workspaces : public miral::TestServer
{
    auto create_window(std::string const& name) -> TestWindow
    {
        TestWindow result;

        result = Surface{mir_connection_create_render_surface_sync(client_connection, 50, 50)};
        result = WindowSpec::for_normal_window(client_connection, 50, 50)
            .set_name(name.c_str())
            .add_surface(result, 50, 50, 0, 0)
            .create_window();

        client_windows[name] = result;
        init_window(result);

        return result;
    }

    void init_window(TestWindow const& window)
    {
        mir::test::Signal window_ready;
        EXPECT_CALL(policy(), advise_window_ready(_)).WillOnce(InvokeWithoutArgs([&]{ window_ready.raise(); }));

        mir_buffer_stream_swap_buffers_sync(
            mir_render_surface_get_buffer_stream(window, 50, 50, mir_pixel_format_argb_8888));

        EXPECT_TRUE(window_ready.wait_for(1s));
    }

    auto create_tip(std::string const& name, Window const& parent) -> TestWindow
    {
        TestWindow result;

        result = Surface{mir_connection_create_render_surface_sync(client_connection, 50, 50)};

        MirRectangle aux_rect{10, 10, 10, 10};
        result = WindowSpec::for_tip(client_connection, 50, 50, parent, &aux_rect, mir_edge_attachment_any)
            .set_name(name.c_str())
            .add_surface(result, 50, 50, 0, 0)
            .create_window();

        client_windows[name] = result;
        init_window(result);

        return result;
    }

    auto create_dialog(std::string const& name, Window const& parent) -> TestWindow
    {
        TestWindow result;

        result = Surface{mir_connection_create_render_surface_sync(client_connection, 50, 50)};

        result = WindowSpec::for_dialog(client_connection, 50, 50, parent)
            .set_name(name.c_str())
            .add_surface(result, 50, 50, 0, 0)
            .create_window();

        client_windows[name] = result;
        init_window(result);

        return result;
    }

    auto create_workspace() -> std::shared_ptr<miral::Workspace>
    {
        std::shared_ptr<miral::Workspace> result;

        invoke_tools([&](WindowManagerTools& tools)
            { result = tools.create_workspace(); });

        return result;
    }

    void SetUp() override
    {
        miral::TestServer::SetUp();
        EXPECT_CALL(policy(), advise_adding_to_workspace(_, _)).Times(AnyNumber());
        EXPECT_CALL(policy(), advise_removing_from_workspace(_, _)).Times(AnyNumber());
        EXPECT_CALL(policy(), advise_focus_gained(_)).Times(AnyNumber());

        client_connection  = connect_client("Workspaces");
        create_window(top_level);
        create_dialog(dialog, client_windows[top_level]);
        create_tip(tip, client_windows[dialog]);

        EXPECT_THAT(client_windows.size(), Eq(3u));
        EXPECT_THAT(server_windows.size(), Eq(3u));
    }

    void TearDown() override
    {
        client_windows.clear();
        client_connection.reset();
        miral::TestServer::TearDown();
    }

    Connection client_connection;

    auto server_window(std::string const& key) -> miral::Window
    {
        std::lock_guard<decltype(mutex)> lock{mutex};
        return server_windows[key];
    }

    auto client_window(std::string const& key) -> Window&
    {
        return client_windows[key];
    }

    auto windows_in_workspace(std::shared_ptr<miral::Workspace> const& workspace) -> std::vector<miral::Window>
    {
        std::vector<miral::Window> result;

        auto enumerate = [&result](miral::Window const& window)
            {
                result.push_back(window);
            };

        invoke_tools([&](WindowManagerTools& tools)
            { tools.for_each_window_in_workspace(workspace, enumerate); });

        return result;
    }

    auto workspaces_containing_window(miral::Window const& window) -> std::vector<std::shared_ptr<miral::Workspace>>
    {
        std::vector<std::shared_ptr<miral::Workspace>> result;

        auto enumerate = [&result](std::shared_ptr<miral::Workspace> const& workspace)
            {
                result.push_back(workspace);
            };

        invoke_tools([&](WindowManagerTools& tools)
            { tools.for_each_workspace_containing(window, enumerate); });

        return result;
    }

    auto policy() -> WorkspacesWindowManagerPolicy&
    {
        if (!the_policy) throw std::logic_error("the_policy isn't valid");
        return *the_policy;
    }

private:
    std::mutex mutable mutex;
    std::map<std::string, TestWindow> client_windows;
    std::map<std::string, miral::Window> server_windows;
    WorkspacesWindowManagerPolicy* the_policy{nullptr};

    friend struct WorkspacesWindowManagerPolicy;

    auto build_window_manager_policy(WindowManagerTools const& tools)
    -> std::unique_ptr<TestWindowManagerPolicy> override
    {
        return std::make_unique<WorkspacesWindowManagerPolicy>(tools, *this);
    }
};

WorkspacesWindowManagerPolicy::WorkspacesWindowManagerPolicy(WindowManagerTools const& tools, Workspaces& test_fixture) :
TestWindowManagerPolicy(tools, test_fixture), test_fixture{test_fixture}
{
    test_fixture.the_policy = this;
}

WorkspacesWindowManagerPolicy::~WorkspacesWindowManagerPolicy()
{
    test_fixture.the_policy = nullptr;
}


void WorkspacesWindowManagerPolicy::advise_new_window(miral::WindowInfo const& window_info)
{
    miral::TestServer::TestWindowManagerPolicy::advise_new_window(window_info);

    std::lock_guard<decltype(test_fixture.mutex)> lock{test_fixture.mutex};
    test_fixture.server_windows[window_info.name()] = window_info.window();
}

void WorkspacesWindowManagerPolicy::handle_window_ready(miral::WindowInfo& window_info)
{
    miral::TestServer::TestWindowManagerPolicy::handle_window_ready(window_info);
    advise_window_ready(window_info);
}
}

TEST_F(Workspaces, before_a_tree_is_added_to_workspace_it_is_empty)
{
    auto const workspace = create_workspace();

    EXPECT_THAT(windows_in_workspace(workspace).size(), Eq(0u));
}

TEST_F(Workspaces, when_a_tree_is_added_to_workspace_all_surfaces_in_tree_are_added)
{
    auto const workspace = create_workspace();
    invoke_tools([&, this](WindowManagerTools& tools)
        { tools.add_tree_to_workspace(server_window(dialog), workspace); });

    EXPECT_THAT(windows_in_workspace(workspace).size(), Eq(3u));
}

TEST_F(Workspaces, when_a_tree_is_removed_from_workspace_all_surfaces_in_tree_are_removed)
{
    auto const workspace = create_workspace();
    invoke_tools([&, this](WindowManagerTools& tools)
        { tools.add_tree_to_workspace(server_window(dialog), workspace); });

    invoke_tools([&, this](WindowManagerTools& tools)
        { tools.remove_tree_from_workspace(server_window(tip), workspace); });

    EXPECT_THAT(windows_in_workspace(workspace).size(), Eq(0u));
}

TEST_F(Workspaces, given_a_tree_in_a_workspace_when_another_tree_is_added_and_removed_from_workspace_the_original_tree_remains)
{
    auto const workspace = create_workspace();
    auto const original_tree = "original_tree";
    auto const client_window = create_window(original_tree);
    auto const original_window= server_window(original_tree);

    invoke_tools([&](WindowManagerTools& tools)
        { tools.add_tree_to_workspace(original_window, workspace); });

    invoke_tools([&](WindowManagerTools& tools)
        { tools.add_tree_to_workspace(server_window(top_level), workspace); });
    invoke_tools([&](WindowManagerTools& tools)
        { tools.remove_tree_from_workspace(server_window(top_level), workspace); });

    EXPECT_THAT(windows_in_workspace(workspace), ElementsAre(original_window));
}

TEST_F(Workspaces, when_a_tree_is_added_to_a_workspace_all_surfaces_are_contained_in_the_workspace)
{
    auto const workspace = create_workspace();
    invoke_tools([&, this](WindowManagerTools& tools)
        { tools.add_tree_to_workspace(server_window(dialog), workspace); });

    EXPECT_THAT(workspaces_containing_window(server_window(top_level)), ElementsAre(workspace));
    EXPECT_THAT(workspaces_containing_window(server_window(dialog)), ElementsAre(workspace));
    EXPECT_THAT(workspaces_containing_window(server_window(tip)), ElementsAre(workspace));
}


TEST_F(Workspaces, when_a_tree_is_added_to_a_workspaces_twice_surfaces_are_contained_in_one_workspace)
{
    auto const workspace = create_workspace();
    invoke_tools([&, this](WindowManagerTools& tools)
        {
            tools.add_tree_to_workspace(server_window(dialog), workspace);
            tools.add_tree_to_workspace(server_window(dialog), workspace);
        });

    EXPECT_THAT(workspaces_containing_window(server_window(top_level)), ElementsAre(workspace));
    EXPECT_THAT(workspaces_containing_window(server_window(dialog)), ElementsAre(workspace));
    EXPECT_THAT(workspaces_containing_window(server_window(tip)), ElementsAre(workspace));

    EXPECT_THAT(workspaces_containing_window(server_window(top_level)).size(), Eq(1u));
    EXPECT_THAT(workspaces_containing_window(server_window(dialog)).size(), Eq(1u));
    EXPECT_THAT(workspaces_containing_window(server_window(tip)).size(), Eq(1u));
}

TEST_F(Workspaces, when_a_tree_is_added_to_two_workspaces_all_surfaces_are_contained_in_two_workspaces)
{
    auto const workspace1 = create_workspace();
    auto const workspace2 = create_workspace();
    invoke_tools([&, this](WindowManagerTools& tools)
        {
            tools.add_tree_to_workspace(server_window(dialog), workspace1);
            tools.add_tree_to_workspace(server_window(dialog), workspace2);
        });

    EXPECT_THAT(workspaces_containing_window(server_window(top_level)).size(), Eq(2u));
    EXPECT_THAT(workspaces_containing_window(server_window(dialog)).size(), Eq(2u));
    EXPECT_THAT(workspaces_containing_window(server_window(tip)).size(), Eq(2u));
}

TEST_F(Workspaces, when_workspace_is_closed_surfaces_are_no_longer_contained_in_it)
{
    auto const workspace1 = create_workspace();
    auto workspace2 = create_workspace();
    invoke_tools([&, this](WindowManagerTools& tools)
        {
            tools.add_tree_to_workspace(server_window(dialog), workspace1);
            tools.add_tree_to_workspace(server_window(dialog), workspace2);
        });

    workspace2.reset();

    EXPECT_THAT(workspaces_containing_window(server_window(top_level)), ElementsAre(workspace1));
    EXPECT_THAT(workspaces_containing_window(server_window(dialog)), ElementsAre(workspace1));
    EXPECT_THAT(workspaces_containing_window(server_window(tip)), ElementsAre(workspace1));

    EXPECT_THAT(workspaces_containing_window(server_window(top_level)).size(), Eq(1u));
    EXPECT_THAT(workspaces_containing_window(server_window(dialog)).size(), Eq(1u));
    EXPECT_THAT(workspaces_containing_window(server_window(tip)).size(), Eq(1u));
}

TEST_F(Workspaces, when_a_tree_is_added_to_a_workspace_the_policy_is_notified)
{
    auto const workspace = create_workspace();

    EXPECT_CALL(policy(), advise_adding_to_workspace(workspace,
         ElementsAre(server_window(top_level), server_window(dialog), server_window(tip))));

    invoke_tools([&, this](WindowManagerTools& tools)
                     { tools.add_tree_to_workspace(server_window(dialog), workspace); });
}

TEST_F(Workspaces, when_a_tree_is_added_to_a_workspaces_twice_the_policy_is_notified_once)
{
    auto const workspace = create_workspace();

    EXPECT_CALL(policy(), advise_adding_to_workspace(workspace,
         ElementsAre(server_window(top_level), server_window(dialog), server_window(tip))));

    invoke_tools([&, this](WindowManagerTools& tools)
        {
            tools.add_tree_to_workspace(server_window(dialog), workspace);
            tools.add_tree_to_workspace(server_window(dialog), workspace);
        });
}

TEST_F(Workspaces, when_a_tree_is_removed_from_a_workspace_the_policy_is_notified)
{
    auto const workspace = create_workspace();
    invoke_tools([&, this](WindowManagerTools& tools)
        { tools.add_tree_to_workspace(server_window(dialog), workspace); });

    EXPECT_CALL(policy(), advise_removing_from_workspace(workspace,
         ElementsAre(server_window(top_level), server_window(dialog), server_window(tip))));

    invoke_tools([&, this](WindowManagerTools& tools)
        { tools.remove_tree_from_workspace(server_window(tip), workspace); });
}

TEST_F(Workspaces, when_a_tree_is_removed_from_a_workspace_twice_the_policy_is_notified_once)
{
    auto const workspace = create_workspace();
    invoke_tools([&, this](WindowManagerTools& tools)
        { tools.add_tree_to_workspace(server_window(dialog), workspace); });

    EXPECT_CALL(policy(), advise_removing_from_workspace(workspace,
         ElementsAre(server_window(top_level), server_window(dialog), server_window(tip))));

    invoke_tools([&, this](WindowManagerTools& tools)
        {
            tools.remove_tree_from_workspace(server_window(top_level), workspace);
            tools.remove_tree_from_workspace(server_window(tip), workspace);
        });
}

TEST_F(Workspaces, a_child_window_is_added_to_workspace_of_parent)
{
    auto const workspace = create_workspace();
    invoke_tools([&, this](WindowManagerTools& tools)
        { tools.add_tree_to_workspace(server_window(dialog), workspace); });

    EXPECT_CALL(policy(), advise_adding_to_workspace(workspace, ElementsAre(_)));

    create_dialog(a_window, client_window(top_level));
}

TEST_F(Workspaces, a_closing_window_is_removed_from_workspace)
{
    auto const workspace = create_workspace();
    invoke_tools([&, this](WindowManagerTools& tools)
        { tools.add_tree_to_workspace(server_window(dialog), workspace); });

    create_dialog(a_window, client_window(dialog));

    EXPECT_CALL(policy(), advise_removing_from_workspace(workspace, ElementsAre(server_window(a_window))));

    client_window(a_window).reset();
}

TEST_F(Workspaces, when_a_window_in_a_workspace_closes_focus_remains_in_workspace)
{
    auto const workspace = create_workspace();

    create_window(a_window);
    create_window(another_window);

    invoke_tools([&, this](WindowManagerTools& tools)
        {
            tools.add_tree_to_workspace(server_window(a_window), workspace);
            tools.add_tree_to_workspace(server_window(another_window), workspace);

            tools.select_active_window(server_window(dialog));
            tools.select_active_window(server_window(a_window));
        });

    client_window(a_window).reset();

    invoke_tools([&, this](WindowManagerTools& tools)
    {
        EXPECT_THAT(tools.active_window(), Eq(server_window(another_window)))
            << "tools.active_window() . . . .: " << tools.info_for(tools.active_window()).name() << "\n"
            << "server_window(another_window): " << tools.info_for(server_window(another_window)).name();
    });
}

TEST_F(Workspaces, with_two_applications_when_a_window_in_a_workspace_closes_focus_remains_in_workspace)
{
    auto const workspace = create_workspace();

    create_window(another_window);

    {
        auto const another_app = connect_client("another app");
        TestWindow window;
        window = Surface{mir_connection_create_render_surface_sync(another_app, 50, 50)};
        window = WindowSpec::for_normal_window(another_app, 50, 50)
            .set_name(a_window.c_str())
            .add_surface(window, 50, 50, 0, 0)
            .create_window();

        init_window(window);

        invoke_tools([&, this](WindowManagerTools& tools)
            {
                tools.add_tree_to_workspace(server_window(top_level), workspace);
                tools.add_tree_to_workspace(server_window(a_window), workspace);
            });
    }

    invoke_tools([&, this](WindowManagerTools& tools)
        {
            EXPECT_THAT(tools.active_window(), Eq(server_window(dialog)))
                << "tools.active_window(): " << tools.info_for(tools.active_window()).name() << "\n"
                << "server_window(dialog): " << tools.info_for(server_window(dialog)).name();
        });
}

TEST_F(Workspaces, when_a_window_in_a_workspace_hides_focus_remains_in_workspace)
{
    auto const workspace = create_workspace();

    create_window(a_window);
    create_window(another_window);

    invoke_tools([&, this](WindowManagerTools& tools)
        {
            tools.add_tree_to_workspace(server_window(a_window), workspace);
            tools.add_tree_to_workspace(server_window(another_window), workspace);

            tools.select_active_window(server_window(dialog));
            tools.select_active_window(server_window(a_window));
        });

    mir::test::Signal focus_changed;
    EXPECT_CALL(policy(), advise_focus_gained(_)).WillOnce(InvokeWithoutArgs([&]{ focus_changed.raise(); }));

    mir_window_set_state(client_window(a_window), mir_window_state_hidden);

    EXPECT_TRUE(focus_changed.wait_for(1s));

    invoke_tools([&, this](WindowManagerTools& tools)
    {
        EXPECT_THAT(tools.active_window(), Eq(server_window(another_window)))
            << "tools.active_window() . . . .: " << tools.info_for(tools.active_window()).name() << "\n"
            << "server_window(another_window): " << tools.info_for(server_window(another_window)).name();
    });
}


TEST_F(Workspaces, with_two_applications_when_a_window_in_a_workspace_hides_focus_remains_in_workspace)
{
    auto const workspace = create_workspace();

    create_window(another_window);

    auto const another_app = connect_client("another app");
    TestWindow window;
    window = Surface{mir_connection_create_render_surface_sync(another_app, 50, 50)};
    window = WindowSpec::for_normal_window(another_app, 50, 50)
        .set_name(a_window.c_str())
        .add_surface(window, 50, 50, 0, 0)
        .create_window();

    init_window(window);

    invoke_tools([&, this](WindowManagerTools& tools)
        {
            tools.add_tree_to_workspace(server_window(top_level), workspace);
            tools.add_tree_to_workspace(server_window(a_window), workspace);
        });


    mir::test::Signal focus_changed;
    EXPECT_CALL(policy(), advise_focus_gained(_)).WillOnce(InvokeWithoutArgs([&]{ focus_changed.raise(); }));

    mir_window_set_state(window, mir_window_state_hidden);

    EXPECT_TRUE(focus_changed.wait_for(1s));

    invoke_tools([&, this](WindowManagerTools& tools)
        {
            EXPECT_THAT(tools.active_window(), Eq(server_window(dialog)))
                << "tools.active_window(): " << tools.info_for(tools.active_window()).name() << "\n"
                << "server_window(dialog): " << tools.info_for(server_window(dialog)).name();
        });

    Mock::VerifyAndClearExpectations(&policy()); // before shutdown
}

TEST_F(Workspaces, focus_next_within_application_keeps_focus_in_workspace)
{
    auto const workspace = create_workspace();

    create_window(another_window);
    create_window(a_window);

    invoke_tools([&, this](WindowManagerTools& tools)
        {
            tools.add_tree_to_workspace(server_window(a_window), workspace);
            tools.add_tree_to_workspace(server_window(dialog), workspace);

            tools.focus_next_within_application();

            EXPECT_THAT(tools.active_window(), Eq(server_window(dialog)))
                << "tools.active_window(): " << tools.info_for(tools.active_window()).name() << "\n"
                << "server_window(dialog): " << tools.info_for(server_window(dialog)).name();

            tools.focus_next_within_application();

            EXPECT_THAT(tools.active_window(), Eq(server_window(a_window)))
                << "tools.active_window(). : " << tools.info_for(tools.active_window()).name() << "\n"
                << "server_window(a_window): " << tools.info_for(server_window(a_window)).name();
        });
}

TEST_F(Workspaces, focus_next_application_keeps_focus_in_workspace)
{
    auto const workspace = create_workspace();
    create_window(another_window);

    auto const another_app = connect_client("another app");
    TestWindow window;
    window = Surface{mir_connection_create_render_surface_sync(another_app, 50, 50)};
    window = WindowSpec::for_normal_window(another_app, 50, 50)
        .set_name(a_window.c_str())
        .add_surface(window, 50, 50, 0, 0)
        .create_window();

    init_window(window);

    invoke_tools([&, this](WindowManagerTools& tools)
        {
            tools.add_tree_to_workspace(server_window(top_level), workspace);
            tools.add_tree_to_workspace(server_window(a_window), workspace);

            tools.focus_next_application();

            EXPECT_THAT(tools.active_window(), Eq(server_window(dialog)))
                << "tools.active_window(): " << tools.info_for(tools.active_window()).name() << "\n"
                << "server_window(dialog): " << tools.info_for(server_window(dialog)).name();

            tools.focus_next_application();

            EXPECT_THAT(tools.active_window(), Eq(server_window(a_window)))
                << "tools.active_window(). : " << tools.info_for(tools.active_window()).name() << "\n"
                << "server_window(a_window): " << tools.info_for(server_window(a_window)).name();
        });
}

TEST_F(Workspaces, move_windows_from_one_workspace_to_another)
{
    auto const pre_workspace = create_workspace();
    auto const from_workspace = create_workspace();
    auto const to_workspace = create_workspace();
    auto const post_workspace = create_workspace();

    create_window(a_window);
    create_window(another_window);

    invoke_tools([&, this](WindowManagerTools& tools)
        {
            tools.add_tree_to_workspace(server_window(a_window), pre_workspace);
            tools.add_tree_to_workspace(server_window(top_level), from_workspace);
            tools.add_tree_to_workspace(server_window(another_window), post_workspace);

            tools.move_workspace_content_to_workspace(to_workspace, from_workspace);
        });

    EXPECT_THAT(windows_in_workspace(from_workspace).size(), Eq(0u));

    EXPECT_THAT(workspaces_containing_window(server_window(a_window)), ElementsAre(pre_workspace));
    EXPECT_THAT(workspaces_containing_window(server_window(top_level)), ElementsAre(to_workspace));
    EXPECT_THAT(workspaces_containing_window(server_window(dialog)), ElementsAre(to_workspace));
    EXPECT_THAT(workspaces_containing_window(server_window(tip)), ElementsAre(to_workspace));
    EXPECT_THAT(workspaces_containing_window(server_window(another_window)), ElementsAre(post_workspace));
}

TEST_F(Workspaces, when_moving_windows_from_one_workspace_to_another_windows_only_appear_once_in_target_workspace)
{
    auto const from_workspace = create_workspace();
    auto const to_workspace = create_workspace();

    create_window(a_window);
    create_window(another_window);

    invoke_tools([&, this](WindowManagerTools& tools)
        {
            tools.add_tree_to_workspace(server_window(a_window), from_workspace);
            tools.add_tree_to_workspace(server_window(another_window), from_workspace);
            tools.add_tree_to_workspace(server_window(a_window), to_workspace);

            tools.move_workspace_content_to_workspace(to_workspace, from_workspace);
        });

    EXPECT_THAT(windows_in_workspace(to_workspace), ElementsAre(server_window(a_window), server_window(another_window)));
}

TEST_F(Workspaces, when_workspace_content_is_moved_the_policy_is_notified)
{
    auto const from_workspace = create_workspace();
    auto const to_workspace = create_workspace();

    EXPECT_CALL(policy(), advise_removing_from_workspace(from_workspace,
         ElementsAre(server_window(top_level), server_window(dialog), server_window(tip))));

     EXPECT_CALL(policy(), advise_adding_to_workspace(to_workspace,
          ElementsAre(server_window(top_level), server_window(dialog), server_window(tip))));

    invoke_tools([&, this](WindowManagerTools& tools)
        {
            tools.add_tree_to_workspace(server_window(dialog), from_workspace);
            tools.move_workspace_content_to_workspace(to_workspace, from_workspace);
        });
}
