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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "src/common/dispatch/utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace md = mir::dispatch;

TEST(DispatchUtilsTest, conversion_functions_roundtrip)
{
    using namespace testing;

    epoll_event dummy;
    for (int event : {EPOLLIN, EPOLLOUT, EPOLLERR})
    {
        dummy.events = event;
        EXPECT_THAT(md::fd_event_to_epoll(md::epoll_to_fd_event(dummy)), Eq(event));
    }

    // HUP is special; FdEvent doesn't differentiate between remote hangup and local hangup
    dummy.events = EPOLLHUP;
    EXPECT_THAT(md::fd_event_to_epoll(md::epoll_to_fd_event(dummy)), Eq(EPOLLHUP | EPOLLRDHUP));
    dummy.events = EPOLLRDHUP;
    EXPECT_THAT(md::fd_event_to_epoll(md::epoll_to_fd_event(dummy)), Eq(EPOLLHUP | EPOLLRDHUP));
}

TEST(DispatchUtilsTest, epoll_to_fd_event_works_for_combinations)
{
    using namespace testing;

    epoll_event dummy;

    dummy.events = EPOLLIN | EPOLLOUT;
    EXPECT_THAT(md::epoll_to_fd_event(dummy), Eq(md::FdEvent::readable | md::FdEvent::writable));

    dummy.events = EPOLLERR | EPOLLOUT;
    EXPECT_THAT(md::epoll_to_fd_event(dummy), Eq(md::FdEvent::error | md::FdEvent::writable));

    dummy.events = EPOLLIN | EPOLLRDHUP;
    EXPECT_THAT(md::epoll_to_fd_event(dummy), Eq(md::FdEvent::readable | md::FdEvent::remote_closed));
}

TEST(DispatchUtilsTest, fd_event_to_epoll_works_for_combinations)
{
    using namespace testing;

    EXPECT_THAT(md::fd_event_to_epoll(md::FdEvent::readable | md::FdEvent::writable),
                Eq(EPOLLIN | EPOLLOUT));
    EXPECT_THAT(md::fd_event_to_epoll(md::FdEvent::error | md::FdEvent::writable),
                Eq(EPOLLERR | EPOLLOUT));
    EXPECT_THAT(md::fd_event_to_epoll(md::FdEvent::readable | md::FdEvent::remote_closed),
                Eq(EPOLLIN | EPOLLHUP | EPOLLRDHUP));
}
