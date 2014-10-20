/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/asio_main_loop.h"
#include "mir/time/high_resolution_clock.h"
#include "mir_test/pipe.h"
#include "mir_test/auto_unblock_thread.h"
#include "mir_test/signal.h"
#include "mir_test/wait_object.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <array>
#include <boost/throw_exception.hpp>

#include <sys/types.h>
#include <unistd.h>

namespace mt = mir::test;

namespace
{

class AsioMainLoopTest : public ::testing::Test
{
public:
    mir::AsioMainLoop ml{std::make_shared<mir::time::HighResolutionClock>()};
};

class AdvanceableClock : public mir::time::Clock
{
public:
    mir::time::Timestamp sample() const override
    {
        std::lock_guard<std::mutex> lock(time_mutex);
        return current_time;
    }
    void advance_by(std::chrono::milliseconds const step, mir::AsioMainLoop & ml)
    {
        bool done = false;
        std::mutex checkpoint_mutex;
        std::condition_variable checkpoint;

        {
            std::lock_guard<std::mutex> lock(time_mutex);
            current_time += step;
        }
        auto evaluate_clock_alarm = ml.notify_in(
            std::chrono::milliseconds{0},
            [&done, &checkpoint_mutex, &checkpoint]
            {
                std::unique_lock<std::mutex> lock(checkpoint_mutex);
                done = true;
                checkpoint.notify_one();
            });

        std::unique_lock<std::mutex> lock(checkpoint_mutex);
        while(!done) checkpoint.wait(lock);

    }
private:
    mutable std::mutex time_mutex;
    mir::time::Timestamp current_time{
        []
        {
           mir::time::HighResolutionClock clock;
           return clock.sample();
        }()
        };
};


class AsioMainLoopAlarmTest : public ::testing::Test
{
public:
    std::shared_ptr<AdvanceableClock> clock = std::make_shared<AdvanceableClock>();
    mir::AsioMainLoop ml{clock};
    int call_count{0};
    mt::WaitObject wait;
    std::chrono::milliseconds delay{50};

};

struct UnblockMainLoop : mt::AutoUnblockThread
{
    UnblockMainLoop(mir::AsioMainLoop & loop)
        : mt::AutoUnblockThread([&loop]() {loop.stop();},
                                [&loop]() {loop.run();})
    {}
};

class Counter
{
public:
    int operator++()
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        cv.notify_one();
        return ++counter;
    }

    bool wait_for(std::chrono::milliseconds const& delay, int expected)
    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        return cv.wait_for(lock, delay, [&]{ return counter == expected;});
    }

    operator int() const
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        return counter;
    }

private:
    std::mutex mutable mutex;
    std::condition_variable cv;
    int counter{0};
};
}

TEST_F(AsioMainLoopTest, signal_handled)
{
    int const signum{SIGUSR1};
    int handled_signum{0};

    ml.register_signal_handler(
        {signum},
        [&handled_signum, this](int sig)
        {
           handled_signum = sig;
           ml.stop();
        });

    kill(getpid(), signum);

    ml.run();

    ASSERT_EQ(signum, handled_signum);
}


TEST_F(AsioMainLoopTest, multiple_signals_handled)
{
    std::vector<int> const signals{SIGUSR1, SIGUSR2};
    size_t const num_signals_to_send{10};
    std::vector<int> handled_signals;
    std::atomic<unsigned int> num_handled_signals{0};

    ml.register_signal_handler(
        {signals[0], signals[1]},
        [&handled_signals, &num_handled_signals](int sig)
        {
           handled_signals.push_back(sig);
           ++num_handled_signals;
        });


    std::thread signal_sending_thread(
        [this, num_signals_to_send, &signals, &num_handled_signals]
        {
            for (size_t i = 0; i < num_signals_to_send; i++)
            {
                kill(getpid(), signals[i % signals.size()]);
                while (num_handled_signals <= i) std::this_thread::yield();
            }
            ml.stop();
        });

    ml.run();

    signal_sending_thread.join();

    ASSERT_EQ(num_signals_to_send, handled_signals.size());

    for (size_t i = 0; i < num_signals_to_send; i++)
        ASSERT_EQ(signals[i % signals.size()], handled_signals[i]) << " index " << i;
}

TEST_F(AsioMainLoopTest, all_registered_handlers_are_called)
{
    int const signum{SIGUSR1};
    std::vector<int> handled_signum{0,0,0};

    ml.register_signal_handler(
        {signum},
        [&handled_signum, this](int sig)
        {
            handled_signum[0] = sig;
            if (handled_signum[0] != 0 &&
                handled_signum[1] != 0 &&
                handled_signum[2] != 0)
            {
                ml.stop();
            }
        });

    ml.register_signal_handler(
        {signum},
        [&handled_signum, this](int sig)
        {
            handled_signum[1] = sig;
            if (handled_signum[0] != 0 &&
                handled_signum[1] != 0 &&
                handled_signum[2] != 0)
            {
                ml.stop();
            }
        });

    ml.register_signal_handler(
        {signum},
        [&handled_signum, this](int sig)
        {
            handled_signum[2] = sig;
            if (handled_signum[0] != 0 &&
                handled_signum[1] != 0 &&
                handled_signum[2] != 0)
            {
                ml.stop();
            }
        });

    kill(getpid(), signum);

    ml.run();

    ASSERT_EQ(signum, handled_signum[0]);
    ASSERT_EQ(signum, handled_signum[1]);
    ASSERT_EQ(signum, handled_signum[2]);
}

TEST_F(AsioMainLoopTest, fd_data_handled)
{
    mt::Pipe p;
    char const data_to_write{'a'};
    int handled_fd{0};
    char data_read{0};

    ml.register_fd_handler(
        {p.read_fd()},
        this,
        [&handled_fd, &data_read, this](int fd)
        {
            handled_fd = fd;
            EXPECT_EQ(1, read(fd, &data_read, 1));
            ml.stop();
        });

    EXPECT_EQ(1, write(p.write_fd(), &data_to_write, 1));

    ml.run();

    EXPECT_EQ(data_to_write, data_read);
}

TEST_F(AsioMainLoopTest, multiple_fds_with_single_handler_handled)
{
    std::vector<mt::Pipe> const pipes(2);
    size_t const num_elems_to_send{10};
    std::vector<int> handled_fds;
    std::vector<size_t> elems_read;
    std::atomic<unsigned int> num_handled_fds{0};

    ml.register_fd_handler(
        {pipes[0].read_fd(), pipes[1].read_fd()},
        this,
        [&handled_fds, &elems_read, &num_handled_fds](int fd)
        {
            handled_fds.push_back(fd);

            size_t i;
            EXPECT_EQ(static_cast<ssize_t>(sizeof(i)),
                      read(fd, &i, sizeof(i)));
            elems_read.push_back(i);

            ++num_handled_fds;
        });

    std::thread fd_writing_thread{
        [this, num_elems_to_send, &pipes, &num_handled_fds]
        {
            for (size_t i = 0; i < num_elems_to_send; i++)
            {
                EXPECT_EQ(static_cast<ssize_t>(sizeof(i)),
                          write(pipes[i % pipes.size()].write_fd(), &i, sizeof(i)));
                while (num_handled_fds <= i) std::this_thread::yield();
            }
            ml.stop();
        }};

    ml.run();

    fd_writing_thread.join();

    ASSERT_EQ(num_elems_to_send, handled_fds.size());
    ASSERT_EQ(num_elems_to_send, elems_read.size());

    for (size_t i = 0; i < num_elems_to_send; i++)
    {
        EXPECT_EQ(pipes[i % pipes.size()].read_fd(), handled_fds[i]) << " index " << i;
        EXPECT_EQ(i, elems_read[i]) << " index " << i;
    }
}

TEST_F(AsioMainLoopTest, multiple_fd_handlers_are_called)
{
    std::vector<mt::Pipe> const pipes(3);
    std::vector<int> const elems_to_send{10,11,12};
    std::vector<int> handled_fds{0,0,0};
    std::vector<int> elems_read{0,0,0};

    ml.register_fd_handler(
        {pipes[0].read_fd()},
        this,
        [&handled_fds, &elems_read, this](int fd)
        {
            EXPECT_EQ(static_cast<ssize_t>(sizeof(elems_read[0])),
                      read(fd, &elems_read[0], sizeof(elems_read[0])));
            handled_fds[0] = fd;
            if (handled_fds[0] != 0 &&
                handled_fds[1] != 0 &&
                handled_fds[2] != 0)
            {
                ml.stop();
            }
        });

    ml.register_fd_handler(
        {pipes[1].read_fd()},
        this,
        [&handled_fds, &elems_read, this](int fd)
        {
            EXPECT_EQ(static_cast<ssize_t>(sizeof(elems_read[1])),
                      read(fd, &elems_read[1], sizeof(elems_read[1])));
            handled_fds[1] = fd;
            if (handled_fds[0] != 0 &&
                handled_fds[1] != 0 &&
                handled_fds[2] != 0)
            {
                ml.stop();
            }
        });

    ml.register_fd_handler(
        {pipes[2].read_fd()},
        this,
        [&handled_fds, &elems_read, this](int fd)
        {
            EXPECT_EQ(static_cast<ssize_t>(sizeof(elems_read[2])),
                      read(fd, &elems_read[2], sizeof(elems_read[2])));
            handled_fds[2] = fd;
            if (handled_fds[0] != 0 &&
                handled_fds[1] != 0 &&
                handled_fds[2] != 0)
            {
                ml.stop();
            }
        });

    EXPECT_EQ(static_cast<ssize_t>(sizeof(elems_to_send[0])),
              write(pipes[0].write_fd(), &elems_to_send[0], sizeof(elems_to_send[0])));
    EXPECT_EQ(static_cast<ssize_t>(sizeof(elems_to_send[1])),
              write(pipes[1].write_fd(), &elems_to_send[1], sizeof(elems_to_send[1])));
    EXPECT_EQ(static_cast<ssize_t>(sizeof(elems_to_send[2])),
              write(pipes[2].write_fd(), &elems_to_send[2], sizeof(elems_to_send[2])));

    ml.run();

    EXPECT_EQ(pipes[0].read_fd(), handled_fds[0]);
    EXPECT_EQ(pipes[1].read_fd(), handled_fds[1]);
    EXPECT_EQ(pipes[2].read_fd(), handled_fds[2]);

    EXPECT_EQ(elems_to_send[0], elems_read[0]);
    EXPECT_EQ(elems_to_send[1], elems_read[1]);
    EXPECT_EQ(elems_to_send[2], elems_read[2]);
}

TEST_F(AsioMainLoopTest, unregister_prevents_callback_and_does_not_harm_other_callbacks)
{
    mt::Pipe p1, p2;
    char const data_to_write{'a'};
    int p2_handler_executes{-1};
    char data_read{0};

    ml.register_fd_handler(
        {p1.read_fd()},
        this,
        [this](int)
        {
            FAIL() << "unregistered handler called";
            ml.stop();
        });

    ml.register_fd_handler(
        {p2.read_fd()},
        this+2,
        [&p2_handler_executes,&data_read,this](int fd)
        {
            p2_handler_executes = fd;
            EXPECT_EQ(1, read(fd, &data_read, 1));
            ml.stop();
        });

    ml.unregister_fd_handler(this);

    EXPECT_EQ(1, write(p1.write_fd(), &data_to_write, 1));
    EXPECT_EQ(1, write(p2.write_fd(), &data_to_write, 1));

    ml.run();

    EXPECT_EQ(data_to_write, data_read);
    EXPECT_EQ(p2.read_fd(), p2_handler_executes);
}

TEST_F(AsioMainLoopTest, unregister_does_not_close_fds)
{
    mt::Pipe p1, p2;
    char const data_to_write{'b'};
    char data_read{0};

    ml.register_fd_handler(
        {p1.read_fd()},
        this,
        [this](int)
        {
            FAIL() << "unregistered handler called";
            ml.stop();
        });

    ml.unregister_fd_handler(this);

    ml.register_fd_handler(
        {p1.read_fd()},
        this,
        [this,&data_read](int fd)
        {
            EXPECT_EQ(1, read(fd, &data_read, 1));
            ml.stop();
        });

    EXPECT_EQ(1, write(p1.write_fd(), &data_to_write, 1));

    ml.run();

    EXPECT_EQ(data_to_write, data_read);
}

TEST_F(AsioMainLoopAlarmTest, main_loop_runs_until_stop_called)
{
    auto mainloop_started = std::make_shared<mt::Signal>();

    auto fire_on_mainloop_start = ml.notify_in(std::chrono::milliseconds{0},
                                               [mainloop_started]()
    {
        mainloop_started->raise();
    });

    UnblockMainLoop unblocker(ml);

    ASSERT_TRUE(mainloop_started->wait_for(std::chrono::milliseconds{100}));

    auto timer_fired = std::make_shared<mt::Signal>();
    auto alarm = ml.notify_in(std::chrono::milliseconds{10}, [timer_fired]
    {
        timer_fired->raise();
    });

    clock->advance_by(std::chrono::milliseconds{10}, ml);
    EXPECT_TRUE(timer_fired->wait_for(std::chrono::milliseconds{500}));

    ml.stop();
    // Main loop should be stopped now

    timer_fired = std::make_shared<mt::Signal>();
    auto should_not_fire =  ml.notify_in(std::chrono::milliseconds{0},
                                         [timer_fired]()
    {
        timer_fired->raise();
    });

    EXPECT_FALSE(timer_fired->wait_for(std::chrono::milliseconds{100}));
}

TEST_F(AsioMainLoopAlarmTest, alarm_starts_in_pending_state)
{
    auto alarm = ml.notify_in(delay, [this]() {});

    UnblockMainLoop unblocker(ml);

    EXPECT_EQ(mir::time::Alarm::pending, alarm->state());
}

TEST_F(AsioMainLoopAlarmTest, alarm_fires_with_correct_delay)
{
    UnblockMainLoop unblocker(ml);

    auto alarm = ml.notify_in(delay, [](){});

    clock->advance_by(delay - std::chrono::milliseconds{1}, ml);
    EXPECT_EQ(mir::time::Alarm::pending, alarm->state());

    clock->advance_by(delay, ml);
    EXPECT_EQ(mir::time::Alarm::triggered, alarm->state());
}

TEST_F(AsioMainLoopAlarmTest, multiple_alarms_fire)
{
    using namespace testing;

    int const alarm_count{10};
    Counter call_count;
    std::array<std::unique_ptr<mir::time::Alarm>, alarm_count> alarms;

    for (auto& alarm : alarms)
        alarm = ml.notify_in(delay, [&call_count](){++call_count;});

    UnblockMainLoop unblocker(ml);
    clock->advance_by(delay, ml);

    call_count.wait_for(delay, alarm_count);
    EXPECT_THAT(call_count, Eq(alarm_count));

    for (auto const& alarm : alarms)
        EXPECT_EQ(mir::time::Alarm::triggered, alarm->state());
}

TEST_F(AsioMainLoopAlarmTest, alarm_changes_to_triggered_state)
{
    auto alarm_fired = std::make_shared<mt::Signal>();
    auto alarm = ml.notify_in(std::chrono::milliseconds{5}, [alarm_fired]()
    {
        alarm_fired->raise();
    });

    UnblockMainLoop unblocker(ml);

    clock->advance_by(delay, ml);
    ASSERT_TRUE(alarm_fired->wait_for(std::chrono::milliseconds{100}));

    EXPECT_EQ(mir::time::Alarm::triggered, alarm->state());
}

TEST_F(AsioMainLoopAlarmTest, cancelled_alarm_doesnt_fire)
{
    UnblockMainLoop unblocker(ml);
    auto alarm = ml.notify_in(std::chrono::milliseconds{100},
                              [](){ FAIL() << "Alarm handler of canceld alarm called";});

    EXPECT_TRUE(alarm->cancel());

    EXPECT_EQ(mir::time::Alarm::cancelled, alarm->state());

    clock->advance_by(std::chrono::milliseconds{100}, ml);

    EXPECT_EQ(mir::time::Alarm::cancelled, alarm->state());
}

TEST_F(AsioMainLoopAlarmTest, destroyed_alarm_doesnt_fire)
{
    auto alarm = ml.notify_in(std::chrono::milliseconds{200},
                              [](){ FAIL() << "Alarm handler of destroyed alarm called"; });

    UnblockMainLoop unblocker(ml);

    alarm.reset(nullptr);
    clock->advance_by(std::chrono::milliseconds{200}, ml);
}

TEST_F(AsioMainLoopAlarmTest, rescheduled_alarm_fires_again)
{
    std::atomic<int> call_count{0};

    auto alarm = ml.notify_in(std::chrono::milliseconds{0}, [&call_count]()
    {
        if (call_count++ > 1)
            FAIL() << "Alarm called too many times";
    });

    UnblockMainLoop unblocker(ml);

    clock->advance_by(std::chrono::milliseconds{0}, ml);
    ASSERT_EQ(mir::time::Alarm::triggered, alarm->state());

    alarm->reschedule_in(std::chrono::milliseconds{100});
    EXPECT_EQ(mir::time::Alarm::pending, alarm->state());

    clock->advance_by(std::chrono::milliseconds{100}, ml);
    EXPECT_EQ(mir::time::Alarm::triggered, alarm->state());
}

TEST_F(AsioMainLoopAlarmTest, rescheduled_alarm_cancels_previous_scheduling)
{
    std::atomic<int> call_count{0};

    auto alarm = ml.notify_in(std::chrono::milliseconds{100}, [&call_count]()
    {
        call_count++;
    });

    UnblockMainLoop unblocker(ml);
    clock->advance_by(std::chrono::milliseconds{90}, ml);

    EXPECT_EQ(mir::time::Alarm::pending, alarm->state());
    EXPECT_EQ(0, call_count);
    EXPECT_TRUE(alarm->reschedule_in(std::chrono::milliseconds{100}));
    EXPECT_EQ(mir::time::Alarm::pending, alarm->state());

    clock->advance_by(std::chrono::milliseconds{110}, ml);

    EXPECT_EQ(mir::time::Alarm::triggered, alarm->state());
    EXPECT_EQ(1, call_count);
}

TEST_F(AsioMainLoopAlarmTest, alarm_callback_cannot_deadlock)
{   // Regression test for deadlock bug LP: #1339700
    std::timed_mutex m;
    std::atomic_bool failed(false);
    int i = 0;
    int const loops = 5;

    auto alarm = ml.notify_in(std::chrono::milliseconds{0}, [&]()
    {
        // From this angle, ensure we can lock m (alarm should be unlocked)
        failed = !m.try_lock_for(std::chrono::seconds{5});
        ASSERT_FALSE(failed);
        ++i;
        m.unlock();
    });

    std::thread t([&]()
        {
            m.lock();
            while (i < loops && !failed)
            {
                // From this angle, ensure we can lock alarm while holding m
                (void)alarm->state();
                m.unlock();
                std::this_thread::yield();
                m.lock();
            }
            m.unlock();
        });

    UnblockMainLoop unblocker(ml);
    for (int j = 0; j < loops; ++j)
    {
        clock->advance_by(std::chrono::milliseconds{11}, ml);
        alarm->reschedule_in(std::chrono::milliseconds{10});
    }

    t.join();
}

TEST_F(AsioMainLoopAlarmTest, alarm_fires_at_correct_time_point)
{
    mir::time::Timestamp real_soon = clock->sample() + std::chrono::milliseconds{120};

    auto alarm = ml.notify_at(real_soon, []{});

    UnblockMainLoop unblocker(ml);

    clock->advance_by(std::chrono::milliseconds{119}, ml);
    EXPECT_EQ(mir::time::Alarm::pending, alarm->state());

    clock->advance_by(std::chrono::milliseconds{1}, ml);
    EXPECT_EQ(mir::time::Alarm::triggered, alarm->state());
}

TEST_F(AsioMainLoopTest, dispatches_action)
{
    using namespace testing;

    int num_actions{0};
    int const owner{0};

    ml.enqueue(
        &owner,
        [&]
        {
            ++num_actions;
            ml.stop();
        });

    ml.run();

    EXPECT_THAT(num_actions, Eq(1));
}

TEST_F(AsioMainLoopTest, dispatches_multiple_actions_in_order)
{
    using namespace testing;

    int const num_actions{5};
    std::vector<int> actions;
    int const owner{0};

    for (int i = 0; i < num_actions; ++i)
    {
        ml.enqueue(
            &owner,
            [&,i]
            {
                actions.push_back(i);
                if (i == num_actions - 1)
                    ml.stop();
            });
    }

    ml.run();

    ASSERT_THAT(actions.size(), Eq(num_actions));
    for (int i = 0; i < num_actions; ++i)
        EXPECT_THAT(actions[i], Eq(i)) << "i = " << i;
}

TEST_F(AsioMainLoopTest, does_not_dispatch_paused_actions)
{
    using namespace testing;

    std::vector<int> actions;
    int const owner1{0};
    int const owner2{0};

    ml.enqueue(
        &owner1,

        [&]
        {
            int const id = 0;
            actions.push_back(id);
        });

    ml.enqueue(
        &owner2,
        [&]
        {
            int const id = 1;
            actions.push_back(id);
        });

    ml.enqueue(
        &owner1,
        [&]
        {
            int const id = 2;
            actions.push_back(id);
        });

    ml.enqueue(
        &owner2,
        [&]
        {
            int const id = 3;
            actions.push_back(id);
            ml.stop();
        });

    ml.pause_processing_for(&owner1);

    ml.run();

    ASSERT_THAT(actions.size(), Eq(2));
    EXPECT_THAT(actions[0], Eq(1));
    EXPECT_THAT(actions[1], Eq(3));
}

TEST_F(AsioMainLoopTest, dispatches_resumed_actions)
{
    using namespace testing;

    std::vector<int> actions;
    void const* const owner1_ptr{&actions};
    int const owner2{0};

    ml.enqueue(
        owner1_ptr,
        [&]
        {
            int const id = 0;
            actions.push_back(id);
            ml.stop();
        });

    ml.enqueue(
        &owner2,
        [&]
        {
            int const id = 1;
            actions.push_back(id);
            ml.resume_processing_for(owner1_ptr);
        });

    ml.pause_processing_for(owner1_ptr);

    ml.run();

    ASSERT_THAT(actions.size(), Eq(2));
    EXPECT_THAT(actions[0], Eq(1));
    EXPECT_THAT(actions[1], Eq(0));
}

TEST_F(AsioMainLoopTest, handles_enqueue_from_within_action)
{
    using namespace testing;

    std::vector<int> actions;
    int const num_actions{10};
    void const* const owner{&num_actions};

    ml.enqueue(
        owner,
        [&]
        {
            int const id = 0;
            actions.push_back(id);
            
            for (int i = 1; i < num_actions; ++i)
            {
                ml.enqueue(
                    owner,
                    [&,i]
                    {
                        actions.push_back(i);
                        if (i == num_actions - 1)
                            ml.stop();
                    });
            }
        });

    ml.run();

    ASSERT_THAT(actions.size(), Eq(num_actions));
    for (int i = 0; i < num_actions; ++i)
        EXPECT_THAT(actions[i], Eq(i)) << "i = " << i;
}

// More targeted regression test for LP: #1381925
TEST_F(AsioMainLoopTest, stress_emits_alarm_notification_with_zero_timeout)
{
    using namespace ::testing;

    UnblockMainLoop unblocker{ml};

    for (int i = 0; i < 1000; ++i)
    {
        mt::WaitObject notification_called;

        auto alarm = ml.notify_in(
            std::chrono::milliseconds{0},
            [&] { notification_called.notify_ready(); });

        notification_called.wait_until_ready(std::chrono::seconds{5});
    }
}
