/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/test/doubles/stub_buffer.h"
#include "src/server/compositor/multi_monitor_arbiter.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <queue>

using namespace testing;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;
namespace mg = mir::graphics;

namespace
{
struct MultiMonitorArbiter : Test
{
    MultiMonitorArbiter()
    {
        for(auto i = 0u; i < num_buffers; i++)
            buffers.emplace_back(std::make_shared<mtd::StubBuffer>());
    }
    unsigned int const num_buffers{6u};
    std::vector<std::shared_ptr<mg::Buffer>> buffers;
    mc::MultiMonitorArbiter arbiter{};
};

MATCHER_P(IsSameBufferAs, buffer, "")
{
    return buffer->id() == arg->id();
}

std::shared_ptr<mg::Buffer> wrap_with_destruction_notifier(
    std::shared_ptr<mg::Buffer> const& buffer,
    std::shared_ptr<bool> const& destroyed)
{
    class DestructionNotifyingBuffer : public mg::Buffer
    {
    public:
        DestructionNotifyingBuffer(
            std::shared_ptr<mg::Buffer> const& buffer,
            std::shared_ptr<bool> const& destroyed)
            : wrapped{buffer},
              destroyed{destroyed}
        {
        }

        ~DestructionNotifyingBuffer()
        {
            *destroyed = true;
        }

        mg::BufferID id() const override
        {
            return wrapped->id();
        }

        mir::geometry::Size size() const override
        {
            return wrapped->size();
        }

        MirPixelFormat pixel_format() const override
        {
            return wrapped->pixel_format();
        }

        mg::NativeBufferBase *native_buffer_base() override
        {
            return wrapped->native_buffer_base();
        }

    private:
        std::shared_ptr<mg::Buffer> const wrapped;
        std::shared_ptr<bool> const destroyed;
    };

    return std::make_shared<DestructionNotifyingBuffer>(buffer, destroyed);
}
}

TEST_F(MultiMonitorArbiter, compositor_access_before_any_submission_throws)
{
    //nothing owned
    EXPECT_THROW({
        arbiter.compositor_acquire(this);
    }, std::logic_error);

    arbiter.submit_buffer(buffers.front());

    //something scheduled, should be ok
    arbiter.compositor_acquire(this);
}

TEST_F(MultiMonitorArbiter, compositor_access)
{
    arbiter.submit_buffer(buffers[0]);
    auto cbuffer = arbiter.compositor_acquire(this);
    EXPECT_THAT(cbuffer, IsSameBufferAs(buffers[0]));
}

TEST_F(MultiMonitorArbiter, compositor_release_sends_buffer_back)
{
    auto buffer_released = std::make_shared<bool>(false);
    auto notifying_buffer = wrap_with_destruction_notifier(buffers[0], buffer_released);
    arbiter.submit_buffer(std::move(notifying_buffer));

    auto cbuffer = arbiter.compositor_acquire(this);
    cbuffer.reset();
    // We need to acquire a new buffer - the current one is on-screen, so can't be sent back.
    arbiter.submit_buffer(buffers[1]);
    arbiter.compositor_acquire(this);

    EXPECT_TRUE(*buffer_released);
}

TEST_F(MultiMonitorArbiter, compositor_can_acquire_different_buffers)
{
    arbiter.submit_buffer(buffers[0]);
    auto cbuffer1 = arbiter.compositor_acquire(this);
    arbiter.submit_buffer(buffers[1]);
    auto cbuffer2 = arbiter.compositor_acquire(this);
    EXPECT_THAT(cbuffer1, Not(IsSameBufferAs(cbuffer2)));
}

TEST_F(MultiMonitorArbiter, compositor_buffer_syncs_to_fastest_compositor)
{
    int comp_id1{0};
    int comp_id2{0};

    arbiter.submit_buffer(buffers[0]);
    auto cbuffer1 = arbiter.compositor_acquire(&comp_id1);
    auto cbuffer2 = arbiter.compositor_acquire(&comp_id2);

    arbiter.submit_buffer(buffers[1]);
    auto cbuffer3 = arbiter.compositor_acquire(&comp_id1);

    arbiter.submit_buffer(buffers[0]);
    auto cbuffer4 = arbiter.compositor_acquire(&comp_id1);
    auto cbuffer5 = arbiter.compositor_acquire(&comp_id2);

    arbiter.submit_buffer(buffers[1]);
    auto cbuffer6 = arbiter.compositor_acquire(&comp_id2);
    auto cbuffer7 = arbiter.compositor_acquire(&comp_id2);

    EXPECT_THAT(cbuffer1, IsSameBufferAs(buffers[0]));
    EXPECT_THAT(cbuffer2, IsSameBufferAs(buffers[0]));
    EXPECT_THAT(cbuffer3, IsSameBufferAs(buffers[1]));
    EXPECT_THAT(cbuffer4, IsSameBufferAs(buffers[0]));
    EXPECT_THAT(cbuffer5, IsSameBufferAs(buffers[0]));
    EXPECT_THAT(cbuffer6, IsSameBufferAs(buffers[1]));
    EXPECT_THAT(cbuffer7, IsSameBufferAs(buffers[1]));
}

TEST_F(MultiMonitorArbiter, compositor_consumes_all_buffers_when_operating_as_a_composited_scene_would)
{
    arbiter.submit_buffer(buffers[0]);
    auto id1 = arbiter.compositor_acquire(this)->id();
    arbiter.submit_buffer(buffers[1]);
    auto id2 = arbiter.compositor_acquire(this)->id();
    arbiter.submit_buffer(buffers[2]);
    auto id3 = arbiter.compositor_acquire(this)->id();
    arbiter.submit_buffer(buffers[3]);
    auto id4 = arbiter.compositor_acquire(this)->id();
    arbiter.submit_buffer(buffers[4]);
    auto iddqd = arbiter.compositor_acquire(this)->id();

    EXPECT_THAT(id1, Eq(buffers[0]->id()));
    EXPECT_THAT(id2, Eq(buffers[1]->id()));
    EXPECT_THAT(id3, Eq(buffers[2]->id()));
    EXPECT_THAT(id4, Eq(buffers[3]->id()));
    EXPECT_THAT(iddqd, Eq(buffers[4]->id()));
}

TEST_F(MultiMonitorArbiter, compositor_consumes_all_buffers_when_operating_as_a_bypassed_buffer_would)
{
    arbiter.submit_buffer(buffers[0]);
    auto cbuffer1 = arbiter.compositor_acquire(this);
    arbiter.submit_buffer(buffers[1]);
    auto cbuffer2 = arbiter.compositor_acquire(this);
    auto id1 = cbuffer1->id();
    cbuffer1.reset();

    arbiter.submit_buffer(buffers[2]);
    auto cbuffer3 = arbiter.compositor_acquire(this);
    auto id2 = cbuffer2->id();
    cbuffer2.reset();

    arbiter.submit_buffer(buffers[3]);
    auto cbuffer4 = arbiter.compositor_acquire(this);
    auto id3 = cbuffer3->id();
    cbuffer3.reset();

    arbiter.submit_buffer(buffers[4]);
    auto cbuffer5 = arbiter.compositor_acquire(this);
    auto id4 = cbuffer4->id();
    cbuffer4.reset();
    auto id5 = cbuffer5->id();
    cbuffer5.reset();

    EXPECT_THAT(id1, Eq(buffers[0]->id()));
    EXPECT_THAT(id2, Eq(buffers[1]->id()));
    EXPECT_THAT(id3, Eq(buffers[2]->id()));
    EXPECT_THAT(id4, Eq(buffers[3]->id()));
    EXPECT_THAT(id5, Eq(buffers[4]->id()));
}

TEST_F(MultiMonitorArbiter, basic_snapshot_equals_compositor_buffer)
{
    arbiter.submit_buffer(buffers[3]);
    auto cbuffer1 = arbiter.compositor_acquire(this);
    arbiter.submit_buffer(buffers[4]);
    auto sbuffer1 = arbiter.snapshot_acquire();
    EXPECT_THAT(cbuffer1, IsSameBufferAs(sbuffer1));
}

TEST_F(MultiMonitorArbiter, basic_snapshot_equals_latest_compositor_buffer)
{
    int that = 4;

    arbiter.submit_buffer(buffers[3]);
    auto cbuffer1 = arbiter.compositor_acquire(this);
    auto cbuffer2 = arbiter.compositor_acquire(&that);
    auto sbuffer1 = arbiter.snapshot_acquire();
    cbuffer2.reset();
    arbiter.submit_buffer(buffers[4]);
    cbuffer2 = arbiter.compositor_acquire(&that);

    auto sbuffer2 = arbiter.snapshot_acquire();
    EXPECT_THAT(cbuffer1, IsSameBufferAs(sbuffer1));
    EXPECT_THAT(cbuffer2, IsSameBufferAs(sbuffer2));
}

TEST_F(MultiMonitorArbiter, snapshot_cycling_doesnt_advance_buffer_for_compositors)
{
    auto that = 4;
    auto a_few_times = 5u;
    arbiter.submit_buffer(buffers[3]);
    auto cbuffer1 = arbiter.compositor_acquire(this);
    arbiter.submit_buffer(buffers[4]);
    std::vector<mg::BufferID> snapshot_buffers(a_few_times);
    for(auto i = 0u; i < a_few_times; i++)
    {
        snapshot_buffers[i] = arbiter.snapshot_acquire()->id();
    }
    auto cbuffer2 = arbiter.compositor_acquire(&that);

    EXPECT_THAT(cbuffer1, IsSameBufferAs(cbuffer2));
    EXPECT_THAT(snapshot_buffers, Each(Eq(cbuffer1->id())));
}

TEST_F(MultiMonitorArbiter, no_buffers_available_throws_on_snapshot)
{
    EXPECT_THROW({
        arbiter.snapshot_acquire();
    }, std::logic_error);
}

TEST_F(MultiMonitorArbiter, snapshotting_will_release_buffer_if_it_was_the_last_owner)
{
    auto buffer_released = std::make_shared<bool>(false);
    arbiter.submit_buffer(wrap_with_destruction_notifier(buffers[3], buffer_released));
    auto cbuffer1 = arbiter.compositor_acquire(this);
    arbiter.submit_buffer(buffers[4]);
    auto sbuffer1 = arbiter.snapshot_acquire();
    cbuffer1.reset();

    // Acquire a new buffer so first one is no longer onscreen.
    arbiter.compositor_acquire(this);

    EXPECT_FALSE(*buffer_released);
    sbuffer1.reset();
    EXPECT_TRUE(*buffer_released);
}

TEST_F(MultiMonitorArbiter, compositor_can_acquire_a_few_times_and_only_sends_on_the_last_release)
{
    int comp_id1{0};
    int comp_id2{0};

    auto buffer_released = std::make_shared<bool>(false);
    arbiter.submit_buffer(wrap_with_destruction_notifier(buffers[0], buffer_released));
    auto cbuffer1 = arbiter.compositor_acquire(&comp_id1);
    arbiter.submit_buffer(buffers[1]);
    auto cbuffer2 = arbiter.compositor_acquire(&comp_id2);
    EXPECT_THAT(cbuffer1, IsSameBufferAs(cbuffer2));

    auto cbuffer3 = arbiter.compositor_acquire(&comp_id1);
    EXPECT_FALSE(*buffer_released);
    cbuffer1.reset();
    cbuffer2.reset();
    EXPECT_TRUE(*buffer_released);
}

TEST_F(MultiMonitorArbiter, advance_on_fastest_has_same_buffer)
{
    int comp_id1{0};
    int comp_id2{0};

    arbiter.submit_buffer(buffers[0]);
    auto id1 = arbiter.compositor_acquire(&comp_id1)->id(); //buffer[0]
    arbiter.submit_buffer(buffers[1]);
    auto id2 = arbiter.compositor_acquire(&comp_id2)->id(); //buffer[0]

    auto cbuffer3 = arbiter.compositor_acquire(&comp_id1); //buffer[1]

    EXPECT_THAT(id1, Eq(id2));
    EXPECT_THAT(id1, Eq(buffers[0]->id()));
    EXPECT_THAT(cbuffer3, IsSameBufferAs(buffers[1]));
}

TEST_F(MultiMonitorArbiter, buffers_are_sent_back)
{
    std::array<std::shared_ptr<bool>, 3> buffer_released = {
        {
            std::make_shared<bool>(false),
            std::make_shared<bool>(false),
            std::make_shared<bool>(false)
        }};
    int comp_id1{0};
    int comp_id2{0};

    std::deque<std::shared_ptr<mg::Buffer>> buf_queue
        {
            wrap_with_destruction_notifier(buffers[0], buffer_released[0]),
            wrap_with_destruction_notifier(buffers[1], buffer_released[1]),
            wrap_with_destruction_notifier(buffers[2], buffer_released[2]),
            buffers[3]
        };


    arbiter.submit_buffer(buf_queue.front());
    buf_queue.pop_front();
    auto b1 = arbiter.compositor_acquire(&comp_id1);
    b1.reset();
    arbiter.submit_buffer(buf_queue.front());
    buf_queue.pop_front();
    auto b2 = arbiter.compositor_acquire(&comp_id1);
    b2.reset();
    arbiter.submit_buffer(buf_queue.front());
    buf_queue.pop_front();
    auto b3 = arbiter.compositor_acquire(&comp_id1);
    auto b5 = arbiter.compositor_acquire(&comp_id2);
    b3.reset();
    arbiter.submit_buffer(buf_queue.front());
    buf_queue.pop_front();
    auto b4 = arbiter.compositor_acquire(&comp_id1);
    b5.reset();
    b4.reset();
    auto b6 = arbiter.compositor_acquire(&comp_id1);
    b6.reset();

    EXPECT_THAT(buffer_released, Each(Pointee(true)));
}

TEST_F(MultiMonitorArbiter, can_check_if_buffers_are_ready)
{
    int comp_id1{0};
    int comp_id2{0};
    arbiter.submit_buffer(buffers[3]);

    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id1));
    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id2));

    auto b1 = arbiter.compositor_acquire(&comp_id1);
    EXPECT_FALSE(arbiter.buffer_ready_for(&comp_id1));
    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id2));
    b1.reset();

    auto b2 = arbiter.compositor_acquire(&comp_id2);
    EXPECT_FALSE(arbiter.buffer_ready_for(&comp_id1));
    EXPECT_FALSE(arbiter.buffer_ready_for(&comp_id2));
}

TEST_F(MultiMonitorArbiter, other_compositor_ready_status_advances_with_fastest_compositor)
{
    int comp_id1{0};
    int comp_id2{0};

    arbiter.submit_buffer(buffers[0]);
    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id1));
    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id2));

    arbiter.compositor_acquire(&comp_id1);
    arbiter.submit_buffer(buffers[1]);
    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id1));
    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id2));

    arbiter.compositor_acquire(&comp_id1);
    arbiter.submit_buffer(buffers[2]);
    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id1));
    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id2));

    arbiter.compositor_acquire(&comp_id1);
    EXPECT_FALSE(arbiter.buffer_ready_for(&comp_id1));
    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id2));

    arbiter.compositor_acquire(&comp_id2);
    EXPECT_FALSE(arbiter.buffer_ready_for(&comp_id1));
    EXPECT_FALSE(arbiter.buffer_ready_for(&comp_id2));
}

TEST_F(MultiMonitorArbiter, will_release_buffer_in_nbuffers_2_starvation_scenario)
{
    int comp_id1{0};
    int comp_id2{0};

    arbiter.submit_buffer(buffers[0]);
    auto b1 = arbiter.compositor_acquire(&comp_id1);
    auto id1 = b1->id();
    arbiter.submit_buffer(buffers[1]);
    auto b2 = arbiter.compositor_acquire(&comp_id1);
    auto id2 = b2->id();

    b1.reset();

    arbiter.submit_buffer(buffers[0]);
    auto b3 = arbiter.compositor_acquire(&comp_id2);
    auto id3 = b3->id();
    auto b4 = arbiter.compositor_acquire(&comp_id2);
    auto id4 = b4->id();

    b3.reset();

    b2.reset();
    b4.reset();

    EXPECT_THAT(id1, Eq(buffers[0]->id()));
    EXPECT_THAT(id2, Eq(buffers[1]->id()));
    EXPECT_THAT(id3, Eq(buffers[1]->id()));
    EXPECT_THAT(id4, Eq(buffers[0]->id()));

}

/**
 * Tests a bug that was never observed, but determined to be in MultiMonitorArbiter before
 * https://github.com/MirServer/mir/pull/862. What was happening was the current_buffer_users set was getting cleared
 * even if the schedule wasn't advancing. To insure this is no longer happening, we follow the specific series of
 * actions that expose the problem:
 * 1. Compositor A acquires a buffer (A gets added to current_buffer_users)
 * 2. Compositor B acquires the same buffer (B get's added to current_buffer_users)
 * 3. Compositor A acquires the buffer again (Since the schedule has nothing left in it, it can not advance. Should not
 *    effect current_buffer_users, but may clear it. Either way re-adds Compositor A to it)
 * 4. A new buffer is added to the schedule
 * 5. Compositor B tries to acquire
 * If the bug exists, Compositor B will have been cleared from current_buffer_users in step 3. Therefore
 * MultiMonitorArbiter will not know it already has been given the current buffer (buffers[0]) and will not advance the
 * schedule, even though there is a new buffer in it.
 */
TEST_F(MultiMonitorArbiter, second_compositor_advances_schedule_after_both_aquire_first_buffer)
{
    int comp_id1{0};
    int comp_id2{1};

    arbiter.submit_buffer(buffers[0]);
    auto cbuffer1 = arbiter.compositor_acquire(&comp_id1);
    auto cbuffer2 = arbiter.compositor_acquire(&comp_id2);
    auto cbuffer3 = arbiter.compositor_acquire(&comp_id1);
    EXPECT_THAT(cbuffer1, IsSameBufferAs(cbuffer2));
    EXPECT_THAT(cbuffer1, IsSameBufferAs(cbuffer3));
    arbiter.submit_buffer(buffers[1]);
    auto cbuffer4 = arbiter.compositor_acquire(&comp_id2);
    EXPECT_THAT(cbuffer1, Not(IsSameBufferAs(cbuffer4)));
}
