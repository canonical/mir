/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/test/doubles/mock_event_sink.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "src/server/compositor/multi_monitor_arbiter.h"
#include "src/server/compositor/schedule.h"

#include <gtest/gtest.h>
using namespace testing;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mf = mir::frontend;

namespace
{
struct FixedSchedule : mc::Schedule
{
    void schedule(std::shared_ptr<mg::Buffer> const&) override
    {
        throw std::runtime_error("this stub doesnt support this");
    }
    unsigned int num_scheduled() override
    {
        return sched.size() - current;
    }
    std::shared_ptr<mg::Buffer> next_buffer() override
    {
        if (sched.empty() || current == sched.size())
            throw std::runtime_error("no buffer scheduled");
        auto buf = sched.front();
        sched.erase(sched.begin());
        return buf;
    }
    void set_schedule(std::vector<std::shared_ptr<mg::Buffer>> s)
    {
        current = 0;
        sched = s;
    }
private:
    unsigned int current{0};
    std::vector<std::shared_ptr<mg::Buffer>> sched;
};

struct MultiMonitorArbiter : Test
{
    MultiMonitorArbiter()
    {
        for(auto i = 0u; i < num_buffers; i++)
            buffers.emplace_back(std::make_shared<mtd::StubBuffer>());
    }
    unsigned int const num_buffers{6u};
    std::vector<std::shared_ptr<mg::Buffer>> buffers;
    FixedSchedule schedule;
    mc::MultiMonitorArbiter arbiter{mt::fake_shared(schedule)};
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

        std::shared_ptr<mg::NativeBuffer> native_buffer_handle() const override
        {
            return wrapped->native_buffer_handle();
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

    schedule.set_schedule({buffers[0]});

    //something scheduled, should be ok
    arbiter.compositor_acquire(this);
}

TEST_F(MultiMonitorArbiter, compositor_access)
{
    schedule.set_schedule({buffers[0]});
    auto cbuffer = arbiter.compositor_acquire(this);
    EXPECT_THAT(cbuffer, IsSameBufferAs(buffers[0]));
}

TEST_F(MultiMonitorArbiter, compositor_release_sends_buffer_back)
{
    auto buffer_released = std::make_shared<bool>(false);
    schedule.set_schedule({ wrap_with_destruction_notifier(buffers[0], buffer_released) });

    auto cbuffer = arbiter.compositor_acquire(this);
    schedule.set_schedule({buffers[1]});
    cbuffer.reset();
    // We need to acquire a new buffer - the current one is on-screen, so can't be sent back.
    arbiter.compositor_acquire(this);
    EXPECT_TRUE(*buffer_released);
}

TEST_F(MultiMonitorArbiter, compositor_can_acquire_different_buffers)
{
    schedule.set_schedule({buffers[0]});
    auto cbuffer1 = arbiter.compositor_acquire(this);
    schedule.set_schedule({buffers[1]});
    auto cbuffer2 = arbiter.compositor_acquire(this);
    EXPECT_THAT(cbuffer1, Not(IsSameBufferAs(cbuffer2)));
}

TEST_F(MultiMonitorArbiter, compositor_buffer_syncs_to_fastest_compositor)
{
    int comp_id1{0};
    int comp_id2{0};

    schedule.set_schedule({buffers[0]});
    auto cbuffer1 = arbiter.compositor_acquire(&comp_id1); 
    auto cbuffer2 = arbiter.compositor_acquire(&comp_id2);

    schedule.set_schedule({buffers[1]});
    auto cbuffer3 = arbiter.compositor_acquire(&comp_id1);

    schedule.set_schedule({buffers[0]});
    auto cbuffer4 = arbiter.compositor_acquire(&comp_id1); 
    auto cbuffer5 = arbiter.compositor_acquire(&comp_id2);

    schedule.set_schedule({buffers[1]});
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
    schedule.set_schedule({buffers[0],buffers[1],buffers[2],buffers[3],buffers[4]});

    auto id1 = arbiter.compositor_acquire(this)->id();
    auto id2 = arbiter.compositor_acquire(this)->id();
    auto id3 = arbiter.compositor_acquire(this)->id();
    auto id4 = arbiter.compositor_acquire(this)->id();
    auto iddqd = arbiter.compositor_acquire(this)->id();

    EXPECT_THAT(id1, Eq(buffers[0]->id()));
    EXPECT_THAT(id2, Eq(buffers[1]->id()));
    EXPECT_THAT(id3, Eq(buffers[2]->id()));
    EXPECT_THAT(id4, Eq(buffers[3]->id()));
    EXPECT_THAT(iddqd, Eq(buffers[4]->id()));
}

TEST_F(MultiMonitorArbiter, compositor_consumes_all_buffers_when_operating_as_a_bypassed_buffer_would)
{
    schedule.set_schedule({buffers[0],buffers[1],buffers[2],buffers[3],buffers[4]});

    auto cbuffer1 = arbiter.compositor_acquire(this);
    auto cbuffer2 = arbiter.compositor_acquire(this);
    auto id1 = cbuffer1->id();
    cbuffer1.reset();

    auto cbuffer3 = arbiter.compositor_acquire(this);
    auto id2 = cbuffer2->id();
    cbuffer2.reset();

    auto cbuffer4 = arbiter.compositor_acquire(this);
    auto id3 = cbuffer3->id();
    cbuffer3.reset();

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

TEST_F(MultiMonitorArbiter, multimonitor_compositor_buffer_syncs_to_fastest_with_more_queueing)
{
    int comp_id1{0};
    int comp_id2{0};

    schedule.set_schedule({buffers[0],buffers[1],buffers[2],buffers[3],buffers[4]});

    auto cbuffer1 = arbiter.compositor_acquire(&comp_id1); //buffer[0]
    auto cbuffer2 = arbiter.compositor_acquire(&comp_id2); //buffer[0]

    auto cbuffer3 = arbiter.compositor_acquire(&comp_id1); //buffer[1]

    auto cbuffer4 = arbiter.compositor_acquire(&comp_id1); //buffer[2]
    auto cbuffer5 = arbiter.compositor_acquire(&comp_id2); //buffer[2]

    auto cbuffer6 = arbiter.compositor_acquire(&comp_id2); //buffer[3]

    auto cbuffer7 = arbiter.compositor_acquire(&comp_id2); //buffer[4]
    auto cbuffer8 = arbiter.compositor_acquire(&comp_id1); //buffer[4]

    EXPECT_THAT(cbuffer1, IsSameBufferAs(buffers[0]));
    EXPECT_THAT(cbuffer2, IsSameBufferAs(buffers[0]));

    EXPECT_THAT(cbuffer3, IsSameBufferAs(buffers[1]));

    EXPECT_THAT(cbuffer4, IsSameBufferAs(buffers[2]));
    EXPECT_THAT(cbuffer5, IsSameBufferAs(buffers[2]));

    EXPECT_THAT(cbuffer6, IsSameBufferAs(buffers[3]));

    EXPECT_THAT(cbuffer7, IsSameBufferAs(buffers[4]));
    EXPECT_THAT(cbuffer8, IsSameBufferAs(buffers[4]));
}

TEST_F(MultiMonitorArbiter, can_set_a_new_schedule)
{
    FixedSchedule another_schedule;
    schedule.set_schedule({buffers[3],buffers[4]});
    another_schedule.set_schedule({buffers[0],buffers[1]});

    auto cbuffer1 = arbiter.compositor_acquire(this);
    arbiter.set_schedule(mt::fake_shared(another_schedule));
    auto cbuffer2 = arbiter.compositor_acquire(this);

    EXPECT_THAT(cbuffer1, IsSameBufferAs(buffers[3]));
    EXPECT_THAT(cbuffer2, IsSameBufferAs(buffers[0]));
}

TEST_F(MultiMonitorArbiter, basic_snapshot_equals_compositor_buffer)
{
    schedule.set_schedule({buffers[3],buffers[4]});

    auto cbuffer1 = arbiter.compositor_acquire(this);
    auto sbuffer1 = arbiter.snapshot_acquire();
    EXPECT_THAT(cbuffer1, IsSameBufferAs(sbuffer1));
}

TEST_F(MultiMonitorArbiter, basic_snapshot_equals_latest_compositor_buffer)
{
    schedule.set_schedule({buffers[3],buffers[4]});
    int that = 4;

    auto cbuffer1 = arbiter.compositor_acquire(this);
    auto cbuffer2 = arbiter.compositor_acquire(&that);
    auto sbuffer1 = arbiter.snapshot_acquire();
    cbuffer2.reset();
    cbuffer2 = arbiter.compositor_acquire(&that);

    auto sbuffer2 = arbiter.snapshot_acquire();
    EXPECT_THAT(cbuffer1, IsSameBufferAs(sbuffer1));
    EXPECT_THAT(cbuffer2, IsSameBufferAs(sbuffer2));
}

TEST_F(MultiMonitorArbiter, snapshot_cycling_doesnt_advance_buffer_for_compositors)
{
    schedule.set_schedule({buffers[3],buffers[4]});
    auto that = 4;
    auto a_few_times = 5u;
    auto cbuffer1 = arbiter.compositor_acquire(this);
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
    schedule.set_schedule({});
    EXPECT_THROW({
        arbiter.snapshot_acquire();
    }, std::logic_error);
}

TEST_F(MultiMonitorArbiter, snapshotting_will_release_buffer_if_it_was_the_last_owner)
{
    auto buffer_released = std::make_shared<bool>(false);
    schedule.set_schedule(
        {
            wrap_with_destruction_notifier(buffers[3], buffer_released),
            buffers[4]
        });
    auto cbuffer1 = arbiter.compositor_acquire(this);
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
    schedule.set_schedule(
        {
            wrap_with_destruction_notifier(buffers[0], buffer_released),
            buffers[1]
        });
    auto cbuffer1 = arbiter.compositor_acquire(&comp_id1);
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
    schedule.set_schedule({buffers[0],buffers[1]});

    auto id1 = arbiter.compositor_acquire(&comp_id1)->id(); //buffer[0]
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

    schedule.set_schedule(
        {
            wrap_with_destruction_notifier(buffers[0], buffer_released[0]),
            wrap_with_destruction_notifier(buffers[1], buffer_released[1]),
            wrap_with_destruction_notifier(buffers[2], buffer_released[2]),
            buffers[3]
        });

    auto b1 = arbiter.compositor_acquire(&comp_id1);
    b1.reset();
    auto b2 = arbiter.compositor_acquire(&comp_id1);
    b2.reset();
    auto b3 = arbiter.compositor_acquire(&comp_id1);
    auto b5 = arbiter.compositor_acquire(&comp_id2);
    b3.reset();
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
    schedule.set_schedule({buffers[3]});

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
    schedule.set_schedule({buffers[0], buffers[1], buffers[2]});

    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id1));
    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id2));

    arbiter.compositor_acquire(&comp_id1);
    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id1));
    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id2));

    arbiter.compositor_acquire(&comp_id1);
    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id1));
    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id2));

    arbiter.compositor_acquire(&comp_id1);
    EXPECT_FALSE(arbiter.buffer_ready_for(&comp_id1));
    EXPECT_TRUE(arbiter.buffer_ready_for(&comp_id2));

    arbiter.compositor_acquire(&comp_id2);
    EXPECT_FALSE(arbiter.buffer_ready_for(&comp_id1));
    EXPECT_FALSE(arbiter.buffer_ready_for(&comp_id2));
}

TEST_F(MultiMonitorArbiter, will_release_buffer_in_nbuffers_2_overlay_scenario)
{
    int comp_id1{0};
    auto buffer_released = std::make_shared<bool>(false);
    auto notifying_buffer = wrap_with_destruction_notifier(buffers[0], buffer_released);
    schedule.set_schedule(
        {
            notifying_buffer,
            buffers[1],
            buffers[0],    // We only want to be notified when the first submission is released
            buffers[1]
        });
    notifying_buffer.reset();

    auto b1 = arbiter.compositor_acquire(&comp_id1);
    auto b2 = arbiter.compositor_acquire(&comp_id1);
    EXPECT_THAT(b1, IsSameBufferAs(buffers[0]));
    EXPECT_THAT(b2, IsSameBufferAs(buffers[1]));
    b1.reset();
    b2.reset();

    EXPECT_TRUE(*buffer_released);
}

TEST_F(MultiMonitorArbiter, will_release_buffer_in_nbuffers_2_starvation_scenario)
{
    int comp_id1{0};
    int comp_id2{0};
    schedule.set_schedule({buffers[0], buffers[1], buffers[0], buffers[1]});

    auto b1 = arbiter.compositor_acquire(&comp_id1);
    auto id1 = b1->id();
    auto b2 = arbiter.compositor_acquire(&comp_id1);
    auto id2 = b2->id();

    b1.reset();

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

TEST_F(MultiMonitorArbiter, will_ensure_smooth_monitor_production)
{
    int comp_id1{0};
    int comp_id2{0};

    schedule.set_schedule({
        buffers[0], buffers[1], buffers[2],
        buffers[0], buffers[1], buffers[2],
        buffers[0], buffers[1], buffers[2]});

    auto b1 = arbiter.compositor_acquire(&comp_id1);
    auto id1 = b1->id();
    auto b2 = arbiter.compositor_acquire(&comp_id2);
    auto id2 = b2->id();
    b1.reset();     // Send nothing

    auto b3 = arbiter.compositor_acquire(&comp_id1);
    auto id3 = b3->id();
    b3.reset();     // Send nothing

    auto b4 = arbiter.compositor_acquire(&comp_id2);
    auto id4 = b4->id();
    b2.reset();     // Send 0

    auto b5 = arbiter.compositor_acquire(&comp_id1);

    EXPECT_THAT(id1, Eq(buffers[0]->id()));
    EXPECT_THAT(id2, Eq(buffers[0]->id()));
    EXPECT_THAT(id3, Eq(buffers[1]->id()));
    EXPECT_THAT(id4, Eq(buffers[1]->id()));
    EXPECT_THAT(b5, IsSameBufferAs(buffers[2]));
}

TEST_F(MultiMonitorArbiter, can_advance_buffer_manually)
{
    int comp_id1{0};
    int comp_id2{0};
    schedule.set_schedule({buffers[0], buffers[1], buffers[2]});

    arbiter.advance_schedule();
    arbiter.advance_schedule();

    auto b1 = arbiter.compositor_acquire(&comp_id1);
    auto b2 = arbiter.compositor_acquire(&comp_id2);
    EXPECT_THAT(b1, IsSameBufferAs(buffers[1]));
    EXPECT_THAT(b2, IsSameBufferAs(buffers[1]));

    auto b3 = arbiter.compositor_acquire(&comp_id1);
    EXPECT_THAT(b3, IsSameBufferAs(buffers[2]));
}

TEST_F(MultiMonitorArbiter, checks_if_buffer_is_valid_after_clean_onscreen_buffer)
{
    int comp_id1{0};

    schedule.set_schedule({buffers[0], buffers[1], buffers[2], buffers[3]});

    arbiter.advance_schedule();
    arbiter.advance_schedule();
    arbiter.advance_schedule();
    arbiter.advance_schedule();

    auto b1 = arbiter.compositor_acquire(&comp_id1);

    EXPECT_THAT(b1->id(), Eq(buffers[3]->id()));
    EXPECT_THAT(b1->size(), Eq(buffers[3]->size()));
}

TEST_F(MultiMonitorArbiter, releases_buffer_on_destruction)
{
    auto buffer_released = std::make_shared<bool>(false);
    schedule.set_schedule({wrap_with_destruction_notifier(buffers[0], buffer_released)});

    {
        mc::MultiMonitorArbiter arbiter{mt::fake_shared(schedule)};
        arbiter.advance_schedule();
    }
    EXPECT_TRUE(*buffer_released);
}

TEST_F(MultiMonitorArbiter, aquires_buffer_after_schedule_runs_out_and_is_refilled)
{
    schedule.set_schedule({buffers[0]});
    auto cbuffer1 = arbiter.compositor_acquire(this);
    auto cbuffer2 = arbiter.compositor_acquire(this);
    EXPECT_THAT(cbuffer1, IsSameBufferAs(cbuffer2));
    schedule.set_schedule({buffers[1]});
    auto cbuffer3 = arbiter.compositor_acquire(this);
    EXPECT_THAT(cbuffer2, Not(IsSameBufferAs(cbuffer3)));
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

    schedule.set_schedule({buffers[0]});
    auto cbuffer1 = arbiter.compositor_acquire(&comp_id1);
    auto cbuffer2 = arbiter.compositor_acquire(&comp_id2);
    auto cbuffer3 = arbiter.compositor_acquire(&comp_id1);
    EXPECT_THAT(cbuffer1, IsSameBufferAs(cbuffer2));
    EXPECT_THAT(cbuffer1, IsSameBufferAs(cbuffer3));
    schedule.set_schedule({buffers[1]});
    auto cbuffer4 = arbiter.compositor_acquire(&comp_id2);
    EXPECT_THAT(cbuffer1, Not(IsSameBufferAs(cbuffer4)));
}
