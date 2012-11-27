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

#include "mir/compositor/buffer_bundle.h"
#include "mir/frontend/session.h"
#include "mir/frontend/session_container.h"
#include "mir/surfaces/surface.h"
#include "mir_test/mock_buffer_bundle.h"
#include "mir_test/empty_deleter.h"
#include "mir_test/mock_surface_organiser.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::surfaces;

TEST(SessionContainer, for_each)
{
    using namespace ::testing;
    std::shared_ptr<mf::SurfaceOrganiser> organiser(new mf::MockSurfaceOrganiser());
    mf::SessionContainer container;

    std::shared_ptr<mf::Session> app1(new mf::Session(organiser, std::string("Visual Studio 7")));
    std::shared_ptr<mf::Session> app2(new mf::Session(organiser, std::string("Visual Studio 8")));

    container.insert_session(app1);
    container.insert_session(app2);

    struct local
    {
        MOCK_METHOD1(check_name, void (std::string const&));

        void operator()(std::shared_ptr<mf::Session> const& session)
        {
            check_name(session->get_name());
        }
    } functor;

    InSequence seq;
    EXPECT_CALL(functor, check_name("Visual Studio 7"));
    EXPECT_CALL(functor, check_name("Visual Studio 8"));

    container.for_each(std::ref(functor));
}
