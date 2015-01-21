/*
 * Copyright © 2014 Canonical Ltd.
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

#include "src/client/rpc/simple_rpc_thread.h"
#include "src/client/rpc/dispatchable.h"
#include "mir/fd.h"
#include "mir_test/pipe.h"
#include "mir_test/signal.h"

#include <fcntl.h>

#include <atomic>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mclr = mir::client::rpc;
namespace mt = mir::test;

namespace
{
class SimpleRPCThreadTest : public ::testing::Test
{
public:
    SimpleRPCThreadTest()
    {
        watch_fd = mir::Fd{pipe.read_fd()};
        test_fd = mir::Fd{pipe.write_fd()};
        fcntl(watch_fd, F_SETFL, O_NONBLOCK);
    }

    mir::Fd watch_fd;
    mir::Fd test_fd;
private:
    mt::Pipe pipe;
};

class MockDispatchable : public mclr::Dispatchable
{
public:
    MOCK_CONST_METHOD0(watch_fd, mir::Fd());
    MOCK_METHOD0(dispatch, void());
};

}

TEST_F(SimpleRPCThreadTest, CallsDispatchWhenFdIsReadable)
{
    using namespace testing;

    auto dispatchable = std::make_shared<NiceMock<MockDispatchable>>();
    ON_CALL(*dispatchable, watch_fd()).WillByDefault(Return(watch_fd));

    auto dispatched = std::make_shared<mt::Signal>();
    ON_CALL(*dispatchable, dispatch()).WillByDefault(Invoke([dispatched]() { dispatched->raise(); }));

    mclr::SimpleRpcThread dispatcher{dispatchable};

    uint64_t dummy{0xdeadbeef};
    EXPECT_EQ(sizeof(dummy), write(test_fd, &dummy, sizeof(dummy)));

    EXPECT_TRUE(dispatched->wait_for(std::chrono::seconds{1}));
}

TEST_F(SimpleRPCThreadTest, StopsCallingDispatchOnceFdIsNotReadable)
{
    using namespace testing;

    uint64_t dummy{0xdeadbeef};
    std::atomic<int> dispatch_count{0};

    auto dispatchable = std::make_shared<NiceMock<MockDispatchable>>();
    ON_CALL(*dispatchable, watch_fd()).WillByDefault(Return(watch_fd));

    auto dispatched = std::make_shared<mt::Signal>();
    ON_CALL(*dispatchable, dispatch()).WillByDefault(Invoke([this, &dispatch_count]()
                                                     {
                                                         decltype(dummy) buffer;
                                                         dispatch_count++;
                                                         read(this->watch_fd, &buffer, sizeof(buffer));
                                                     }));

    mclr::SimpleRpcThread dispatcher{dispatchable};

    EXPECT_EQ(sizeof(dummy), write(test_fd, &dummy, sizeof(dummy)));

    std::this_thread::sleep_for(std::chrono::seconds{1});

    EXPECT_EQ(1, dispatch_count);
}
