/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/frontend/session.h"
#include "mir/frontend/shell.h"

#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/session.h"
#include "mir/shell/focus_controller.h"

#include "mir/scene/surface.h"
#include "src/server/scene/surface_stack.h"

#include "mir/test/doubles/null_event_sink.h"
#include "mir_test_framework/stubbed_server_configuration.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

namespace mf = mir::frontend;
namespace mo = mir::options;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
struct TestSurfaceStack : public ms::SurfaceStack
{
    using ms::SurfaceStack::SurfaceStack;

    MOCK_METHOD3(add_surface, void(
        std::shared_ptr<ms::Surface> const& surface,
        ms::DepthId depth,
        mir::input::InputReceptionMode input_mode));

    MOCK_METHOD1(raise, void(
        std::weak_ptr<ms::Surface> const& surface));

    void default_add_surface(
        std::shared_ptr<ms::Surface> const& surface,
        ms::DepthId depth,
        mir::input::InputReceptionMode input_mode)
    {
        ms::SurfaceStack::add_surface(surface, depth, input_mode);
    }

    void default_raise(std::weak_ptr<ms::Surface> const& surface)
    {
        ms::SurfaceStack::raise(surface);
    }

};

struct TestConfiguration : public mir_test_framework::StubbedServerConfiguration
{

    std::shared_ptr<ms::SurfaceStackModel>
    the_surface_stack_model() override
    {
        return surface_stack(
            [this]()
            {
                auto const scene_report = the_scene_report();

                test_surface_stack = std::make_shared<TestSurfaceStack>(scene_report);

                return test_surface_stack;
            });
    }

    std::shared_ptr<TestSurfaceStack> test_surface_stack;
};

struct SessionManagement : Test
{
    TestConfiguration builder;
    std::shared_ptr<mf::EventSink> const event_sink = std::make_shared<mtd::NullEventSink>();
    std::shared_ptr<mf::Shell> const session_manager = builder.the_frontend_shell();
    std::shared_ptr<TestSurfaceStack> const& test_surface_stack = builder.test_surface_stack;
    ms::SurfaceCreationParameters const params = ms::SurfaceCreationParameters().of_size(100,100).of_type(mir_surface_type_normal);

    void SetUp()
    {
        ASSERT_THAT(test_surface_stack, Ne(nullptr));
        ON_CALL(*test_surface_stack, add_surface(_,_,_))
            .WillByDefault(Invoke(test_surface_stack.get(), &TestSurfaceStack::default_add_surface));
        ON_CALL(*test_surface_stack, raise(_))
            .WillByDefault(Invoke(test_surface_stack.get(), &TestSurfaceStack::default_raise));
    }
};

MATCHER_P(WeakPtrTo, p, "")
{
  return arg.lock() == p;
}
}

TEST_F(SessionManagement, creating_a_surface_adds_it_to_scene)
{
    auto const session = session_manager->open_session(0, __PRETTY_FUNCTION__, event_sink);

    EXPECT_CALL(*test_surface_stack, add_surface(_,_,_)).Times(1);
    session_manager->create_surface(session, params, nullptr);
}
