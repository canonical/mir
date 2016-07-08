/*
 * Copyright © 2016 Canonical Ltd.
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

#include "src/client/no_tls_future-inl.h"

#include <gtest/gtest.h>

namespace mcl = mir::client;

TEST(NoTLSFuture, and_then_calls_back_immediately_when_promise_is_already_fulfilled)
{
    mcl::NoTLSPromise<int> promise;
    promise.set_value(1);

    auto future = promise.get_future();
    bool callback_called{false};

    future.and_then([&callback_called](int) { callback_called = true;});
    EXPECT_TRUE(callback_called);
}
