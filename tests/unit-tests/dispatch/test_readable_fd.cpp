/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreasd Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/dispatch/readable_fd.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace md = mir::dispatch;

TEST(ReadableFd, executes_action_when_readable)
{
    using namespace testing;
    bool action_triggered = false;
    md::ReadableFd dispatchable(mir::Fd{mir::IntOwnedFd{0}},
                                [&action_triggered](){ action_triggered = true;});
    dispatchable.dispatch(md::FdEvent::readable);
    EXPECT_THAT(action_triggered, Eq(true));
}
