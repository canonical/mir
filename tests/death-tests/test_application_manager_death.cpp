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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/shell/session_manager.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace msh = mir::shell;

TEST(SessionManagerDeathTest, DISABLED_class_invariants_not_satisfied_triggers_assertion)
{
// Trying to avoid "[WARNING] /usr/src/gtest/src/gtest-death-test.cc:789::
// Death tests use fork(), which is unsafe particularly in a threaded context.
// For this test, Google Test couldn't detect the number of threads." by
//  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
// leads to the test failing under valgrind
    EXPECT_EXIT(
                std::shared_ptr<msh::SurfaceFactory> surface_factory;
                mir::shell::SessionManager app(surface_factory,
                                               std::shared_ptr<msh::SessionContainer>(),
                                               std::shared_ptr<msh::FocusSequence>(),
                                               std::shared_ptr<msh::FocusSetter>(),
                                               std::shared_ptr<msh::InputTargetListener>()),
                ::testing::KilledBySignal(SIGABRT),
                ".*");
}




