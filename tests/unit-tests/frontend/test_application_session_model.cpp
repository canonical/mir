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
#include "mir/frontend/application_session.h"
#include "mir/frontend/the_session_container_implementation.h"
#include "mir/surfaces/surface.h"
#include "mir_test/mock_buffer_bundle.h"
#include "mir_test/empty_deleter.h"
#include "mir_test/mock_application_surface_organiser.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::surfaces;

TEST(TheSessionContainerImplementation, iterate_registration_order)
{
    using namespace ::testing;
    std::shared_ptr<mf::SurfaceOrganiser> organiser(new mf::MockSurfaceOrganiser());
    mf::TheSessionContainerImplementation container;

    std::shared_ptr<mf::Session> app1(new mf::Session(organiser, std::string("Visual Studio 7")));
    std::shared_ptr<mf::Session> app2(new mf::Session(organiser, std::string("Visual Studio 8")));

    container.insert_session(app1);
    container.insert_session(app2);

    auto it = container.iterator();

    EXPECT_EQ("Visual Studio 7", (**it)->get_name());
    it->advance();
    EXPECT_EQ("Visual Studio 8", (**it)->get_name());
    it->advance();
    EXPECT_FALSE(it->is_valid());
    it->reset();
    EXPECT_EQ("Visual Studio 7", (**it)->get_name());
}
