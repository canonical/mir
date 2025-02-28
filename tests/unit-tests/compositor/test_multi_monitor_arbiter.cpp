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

#include "mir/geometry/forward.h"
#include "mir/test/doubles/stub_buffer.h"
#include "src/server/compositor/multi_monitor_arbiter.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <deque>

using namespace testing;
namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

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
    std::shared_ptr<mc::MultiMonitorArbiter> arbiter{std::make_shared<mc::MultiMonitorArbiter>()};
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

namespace
{
auto default_submission_data_from_buffer(std::shared_ptr<mg::Buffer> buffer)
    -> std::tuple<std::shared_ptr<mg::Buffer>, geom::Size, geom::RectangleD>
{
    return std::make_tuple(
        buffer,
        buffer->size(),
        geom::RectangleD{{0, 0}, geom::SizeD{buffer->size()}});
}
}

TEST_F(MultiMonitorArbiter, compositor_access_before_any_submission_throws)
{
    //nothing owned
    EXPECT_THROW({
        arbiter->compositor_acquire(this);
    }, std::logic_error);

    auto [buffer, size, source] = default_submission_data_from_buffer(buffers.front());
    arbiter->submit_buffer(buffer, size, source);

    //something scheduled, should be ok
    arbiter->compositor_acquire(this);
}

TEST_F(MultiMonitorArbiter, compositor_access)
{
    auto [buffer, size, source] = default_submission_data_from_buffer(buffers[0]);
    arbiter->submit_buffer(buffer, size, source);
    auto cbuffer = arbiter->compositor_acquire(this);
    EXPECT_THAT(cbuffer->claim_buffer(), IsSameBufferAs(buffers[0]));
}

TEST_F(MultiMonitorArbiter, compositor_release_sends_buffer_back)
{
    auto buffer_released = std::make_shared<bool>(false);
    auto notifying_buffer = wrap_with_destruction_notifier(buffers[0], buffer_released);
    auto [buffer, size, source] = default_submission_data_from_buffer(std::move(notifying_buffer));
    arbiter->submit_buffer(std::move(buffer), size, source);

    auto cbuffer = arbiter->compositor_acquire(this);
    cbuffer->claim_buffer();
    cbuffer.reset();
    // We need to acquire a new buffer - the current one is on-screen, so can't be sent back.
    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[1]);
    arbiter->submit_buffer(buffer, size, source);
    arbiter->compositor_acquire(this)->claim_buffer();

    EXPECT_TRUE(*buffer_released);
}

TEST_F(MultiMonitorArbiter, compositor_can_acquire_different_buffers)
{
    auto [buffer, size, source] = default_submission_data_from_buffer(buffers[0]);
    arbiter->submit_buffer(buffer, size, source);
    auto cbuffer1 = arbiter->compositor_acquire(this)->claim_buffer();
    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[1]);
    arbiter->submit_buffer(buffer, size, source);
    auto cbuffer2 = arbiter->compositor_acquire(this)->claim_buffer();
    EXPECT_THAT(cbuffer1, Not(IsSameBufferAs(cbuffer2)));
}

TEST_F(MultiMonitorArbiter, compositor_buffer_syncs_to_fastest_compositor)
{
    int comp_id1{0};
    int comp_id2{0};

    auto [buffer, size, source] = default_submission_data_from_buffer(buffers[0]);
    arbiter->submit_buffer(buffer, size, source);
    auto cbuffer1 = arbiter->compositor_acquire(&comp_id1)->claim_buffer();
    auto cbuffer2 = arbiter->compositor_acquire(&comp_id2)->claim_buffer();

    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[1]);
    arbiter->submit_buffer(buffer, size, source);
    auto cbuffer3 = arbiter->compositor_acquire(&comp_id1)->claim_buffer();

    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[0]);
    arbiter->submit_buffer(buffer, size, source);
    auto cbuffer4 = arbiter->compositor_acquire(&comp_id1)->claim_buffer();
    auto cbuffer5 = arbiter->compositor_acquire(&comp_id2)->claim_buffer();

    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[1]);
    arbiter->submit_buffer(buffer, size, source);
    auto cbuffer6 = arbiter->compositor_acquire(&comp_id2)->claim_buffer();
    auto cbuffer7 = arbiter->compositor_acquire(&comp_id2)->claim_buffer();

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
    auto [buffer, size, source] = default_submission_data_from_buffer(buffers[0]);
    arbiter->submit_buffer(buffer, size, source);
    auto id1 = arbiter->compositor_acquire(this)->claim_buffer()->id();

    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[1]);
    arbiter->submit_buffer(buffer, size, source);
    auto id2 = arbiter->compositor_acquire(this)->claim_buffer()->id();

    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[2]);
    arbiter->submit_buffer(buffer, size, source);
    auto id3 = arbiter->compositor_acquire(this)->claim_buffer()->id();

    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[3]);
    arbiter->submit_buffer(buffer, size, source);
    auto id4 = arbiter->compositor_acquire(this)->claim_buffer()->id();

    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[4]);
    arbiter->submit_buffer(buffer, size, source);
    auto iddqd = arbiter->compositor_acquire(this)->claim_buffer()->id();

    EXPECT_THAT(id1, Eq(buffers[0]->id()));
    EXPECT_THAT(id2, Eq(buffers[1]->id()));
    EXPECT_THAT(id3, Eq(buffers[2]->id()));
    EXPECT_THAT(id4, Eq(buffers[3]->id()));
    EXPECT_THAT(iddqd, Eq(buffers[4]->id()));
}

TEST_F(MultiMonitorArbiter, compositor_consumes_all_buffers_when_operating_as_a_bypassed_buffer_would)
{
    auto [buffer, size, source] = default_submission_data_from_buffer(buffers[0]);
    arbiter->submit_buffer(buffer, size, source);
    auto cbuffer1 = arbiter->compositor_acquire(this)->claim_buffer();

    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[1]);
    arbiter->submit_buffer(buffer, size, source);
    auto cbuffer2 = arbiter->compositor_acquire(this)->claim_buffer();
    auto id1 = cbuffer1->id();

    cbuffer1.reset();

    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[2]);
    arbiter->submit_buffer(buffer, size, source);
    auto cbuffer3 = arbiter->compositor_acquire(this)->claim_buffer();
    auto id2 = cbuffer2->id();
    cbuffer2.reset();

    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[3]);
    arbiter->submit_buffer(buffer, size, source);
    auto cbuffer4 = arbiter->compositor_acquire(this)->claim_buffer();
    auto id3 = cbuffer3->id();
    cbuffer3.reset();

    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[4]);
    arbiter->submit_buffer(buffer, size, source);
    auto cbuffer5 = arbiter->compositor_acquire(this)->claim_buffer();
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

TEST_F(MultiMonitorArbiter, compositor_can_acquire_a_few_times_and_only_sends_on_the_last_release)
{
    int comp_id1{0};
    int comp_id2{0};

    auto buffer_released = std::make_shared<bool>(false);
    auto [buffer, size, source] =
        default_submission_data_from_buffer(wrap_with_destruction_notifier(buffers[0], buffer_released));
    arbiter->submit_buffer(std::move(buffer), size, source);
    auto cbuffer1 = arbiter->compositor_acquire(&comp_id1)->claim_buffer();

    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[1]);
    arbiter->submit_buffer(buffer, size, source);
    auto cbuffer2 = arbiter->compositor_acquire(&comp_id2)->claim_buffer();
    EXPECT_THAT(cbuffer1, IsSameBufferAs(cbuffer2));

    auto cbuffer3 = arbiter->compositor_acquire(&comp_id1)->claim_buffer();
    EXPECT_FALSE(*buffer_released);
    cbuffer1.reset();
    cbuffer2.reset();
    EXPECT_TRUE(*buffer_released);
}

TEST_F(MultiMonitorArbiter, advance_on_fastest_has_same_buffer)
{
    int comp_id1{0};
    int comp_id2{0};

    auto [buffer, size, source] = default_submission_data_from_buffer(buffers[0]);
    arbiter->submit_buffer(buffer, size, source);
    auto id1 = arbiter->compositor_acquire(&comp_id1)->claim_buffer()->id(); //buffer[0]
    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[1]);
    arbiter->submit_buffer(buffer, size, source);
    auto id2 = arbiter->compositor_acquire(&comp_id2)->claim_buffer()->id(); //buffer[0]

    auto cbuffer3 = arbiter->compositor_acquire(&comp_id1)->claim_buffer(); //buffer[1]

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


    auto [buffer, size, source] = default_submission_data_from_buffer(buf_queue.front());
    arbiter->submit_buffer(buffer, size, source);
    buf_queue.pop_front();
    auto b1 = arbiter->compositor_acquire(&comp_id1)->claim_buffer();
    b1.reset();
    std::tie(buffer, size, source) = default_submission_data_from_buffer(buf_queue.front());
    arbiter->submit_buffer(buffer, size, source);
    buf_queue.pop_front();
    auto b2 = arbiter->compositor_acquire(&comp_id1)->claim_buffer();
    b2.reset();
    std::tie(buffer, size, source) = default_submission_data_from_buffer(buf_queue.front());
    arbiter->submit_buffer(buffer, size, source);
    buf_queue.pop_front();
    auto b3 = arbiter->compositor_acquire(&comp_id1)->claim_buffer();
    auto b5 = arbiter->compositor_acquire(&comp_id2)->claim_buffer();
    b3.reset();
    std::tie(buffer, size, source) = default_submission_data_from_buffer(buf_queue.front());
    arbiter->submit_buffer(buffer, size, source);
    buf_queue.pop_front();
    auto b4 = arbiter->compositor_acquire(&comp_id1)->claim_buffer();
    b5.reset();
    b4.reset();
    auto b6 = arbiter->compositor_acquire(&comp_id1)->claim_buffer();
    b6.reset();

    EXPECT_THAT(buffer_released, Each(Pointee(true)));
}

TEST_F(MultiMonitorArbiter, will_release_buffer_in_nbuffers_2_starvation_scenario)
{
    int comp_id1{0};
    int comp_id2{0};

    auto [buffer, size, source] = default_submission_data_from_buffer(buffers[0]);
    arbiter->submit_buffer(buffer, size, source);
    auto b1 = arbiter->compositor_acquire(&comp_id1)->claim_buffer();
    auto id1 = b1->id();
    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[1]);
    arbiter->submit_buffer(buffer, size, source);
    auto b2 = arbiter->compositor_acquire(&comp_id1)->claim_buffer();
    auto id2 = b2->id();

    b1.reset();

    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[0]);
    arbiter->submit_buffer(buffer, size, source);
    auto b3 = arbiter->compositor_acquire(&comp_id2)->claim_buffer();
    auto id3 = b3->id();
    auto b4 = arbiter->compositor_acquire(&comp_id2)->claim_buffer();
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

    auto [buffer, size, source] = default_submission_data_from_buffer(buffers[0]);
    arbiter->submit_buffer(buffer, size, source);
    auto cbuffer1 = arbiter->compositor_acquire(&comp_id1)->claim_buffer();
    auto cbuffer2 = arbiter->compositor_acquire(&comp_id2)->claim_buffer();
    auto cbuffer3 = arbiter->compositor_acquire(&comp_id1)->claim_buffer();
    EXPECT_THAT(cbuffer1, IsSameBufferAs(cbuffer2));
    EXPECT_THAT(cbuffer1, IsSameBufferAs(cbuffer3));
    std::tie(buffer, size, source) = default_submission_data_from_buffer(buffers[1]);
    arbiter->submit_buffer(buffer, size, source);
    auto cbuffer4 = arbiter->compositor_acquire(&comp_id2)->claim_buffer();
    EXPECT_THAT(cbuffer1, Not(IsSameBufferAs(cbuffer4)));
}
