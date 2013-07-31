/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#include "mir/shell/application_session.h"
#include "mir/shell/default_session_container.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/shell/null_session_listener.h"
#include "mir/surfaces/surface.h"
#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/mock_surface_factory.h"
#include "mir_test_doubles/null_snapshot_strategy.h"
#include "mir_test_doubles/null_event_sink.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace mtd = mir::test::doubles;

namespace
{

std::shared_ptr<msh::ApplicationSession> make_session(
    std::shared_ptr<msh::SurfaceFactory> const& factory,
    std::string const& session_name)
{
    return std::make_shared<msh::ApplicationSession>(
        factory, session_name,
        std::make_shared<mtd::NullSnapshotStrategy>(),
        std::make_shared<msh::NullSessionListener>(),
        std::make_shared<mtd::NullEventSink>());
}

}

TEST(DefaultSessionContainer, for_each)
{
    using namespace ::testing;
    auto factory = std::make_shared<mtd::MockSurfaceFactory>();
    msh::DefaultSessionContainer container;

    container.insert_session(make_session(factory, "Visual Studio 7"));
    container.insert_session(make_session(factory, "Visual Studio 8"));

    struct local
    {
        MOCK_METHOD1(check_name, void (std::string const&));

        void operator()(std::shared_ptr<mf::Session> const& session)
        {
            check_name(session->name());
        }
    } functor;

    InSequence seq;
    EXPECT_CALL(functor, check_name("Visual Studio 7"));
    EXPECT_CALL(functor, check_name("Visual Studio 8"));

    container.for_each(std::ref(functor));
}

TEST(DefaultSessionContainer, invalid_session_throw_behavior)
{
    using namespace ::testing;
    auto factory = std::make_shared<mtd::MockSurfaceFactory>();
    msh::DefaultSessionContainer container;

    auto session = make_session(factory, "Visual Studio 7");
    EXPECT_THROW({
        container.remove_session(session);
    }, std::logic_error);
}
