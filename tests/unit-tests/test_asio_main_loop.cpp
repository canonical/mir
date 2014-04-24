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
#include "mir_test_framework/watchdog.h"

#include <gtest/gtest.h>

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
namespace mtf = mir_test_framework;

TEST(AsioMainLoopTest, signal_handled)
{
    int const signum{SIGUSR1};
    int handled_signum{0};

    mir::AsioMainLoop ml;

    ml.register_signal_handler(
        {signum},
        [&handled_signum, &ml](int sig)
        {
           handled_signum = sig;
           ml.stop();
        });

    kill(getpid(), signum);

    ml.run();

    ASSERT_EQ(signum, handled_signum);
}


TEST(AsioMainLoopTest, multiple_signals_handled)
{
    std::vector<int> const signals{SIGUSR1, SIGUSR2};
    size_t const num_signals_to_send{10};
    std::vector<int> handled_signals;
    std::atomic<unsigned int> num_handled_signals{0};

    mir::AsioMainLoop ml;

    ml.register_signal_handler(
        {signals[0], signals[1]},
        [&handled_signals, &num_handled_signals](int sig)
        {
           handled_signals.push_back(sig);
           ++num_handled_signals;
        });


    std::thread signal_sending_thread(
        [&ml, num_signals_to_send, &signals, &num_handled_signals]
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

TEST(AsioMainLoopTest, all_registered_handlers_are_called)
{
    int const signum{SIGUSR1};
    std::vector<int> handled_signum{0,0,0};

    mir::AsioMainLoop ml;

    ml.register_signal_handler(
        {signum},
        [&handled_signum, &ml](int sig)
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
        [&handled_signum, &ml](int sig)
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
        [&handled_signum, &ml](int sig)
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

TEST(AsioMainLoopTest, fd_data_handled)
{
    mt::Pipe p;
    char const data_to_write{'a'};
    int handled_fd{0};
    char data_read{0};

    mir::AsioMainLoop ml;

    ml.register_fd_handler(
        {p.read_fd()},
        [&handled_fd, &data_read, &ml](int fd)
        {
            handled_fd = fd;
            EXPECT_EQ(1, read(fd, &data_read, 1));
            ml.stop();
        });

    EXPECT_EQ(1, write(p.write_fd(), &data_to_write, 1));

    ml.run();

    EXPECT_EQ(data_to_write, data_read);
}

TEST(AsioMainLoopTest, multiple_fds_with_single_handler_handled)
{
    std::vector<mt::Pipe> const pipes(2);
    size_t const num_elems_to_send{10};
    std::vector<int> handled_fds;
    std::vector<size_t> elems_read;
    std::atomic<unsigned int> num_handled_fds{0};

    mir::AsioMainLoop ml;

    ml.register_fd_handler(
        {pipes[0].read_fd(), pipes[1].read_fd()},
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
        [&ml, num_elems_to_send, &pipes, &num_handled_fds]
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

TEST(AsioMainLoopTest, multiple_fd_handlers_are_called)
{
    std::vector<mt::Pipe> const pipes(3);
    std::vector<int> const elems_to_send{10,11,12};
    std::vector<int> handled_fds{0,0,0};
    std::vector<int> elems_read{0,0,0};

    mir::AsioMainLoop ml;

    ml.register_fd_handler(
        {pipes[0].read_fd()},
        [&handled_fds, &elems_read, &ml](int fd)
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
        [&handled_fds, &elems_read, &ml](int fd)
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
        [&handled_fds, &elems_read, &ml](int fd)
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

TEST(AsioMainLoopTest, main_loop_runs_until_stop_called)
{
    mir::AsioMainLoop ml;

    mtf::WatchDog runner([&ml]() {ml.stop();});

    std::mutex checkpoint_mutex;
    std::condition_variable checkpoint;
    bool hit_checkpoint{false};

    auto fire_on_mainloop_start = ml.notify_in(std::chrono::milliseconds{0},
                                               [&checkpoint_mutex, &checkpoint, &hit_checkpoint]()
    {
        std::unique_lock<decltype(checkpoint_mutex)> lock(checkpoint_mutex);
        hit_checkpoint = true;
        checkpoint.notify_all();
    });

    runner.run([&ml](mtf::WatchDog&) { ml.run(); });

    {
        std::unique_lock<decltype(checkpoint_mutex)> lock(checkpoint_mutex);
        ASSERT_TRUE(checkpoint.wait_for(lock, std::chrono::milliseconds{10}, [&hit_checkpoint]() { return hit_checkpoint; }));
    }

    auto alarm = ml.notify_in(std::chrono::milliseconds{10}, [&runner]
    {
        runner.notify_done();
    });

    EXPECT_TRUE(runner.wait_for(std::chrono::milliseconds{50}));

    ml.stop();
    // Main loop should be stopped now

    hit_checkpoint = false;
    auto should_not_fire =  ml.notify_in(std::chrono::milliseconds{0},
                                         [&checkpoint_mutex, &checkpoint, &hit_checkpoint]()
    {
        std::unique_lock<decltype(checkpoint_mutex)> lock(checkpoint_mutex);
        hit_checkpoint = true;
        checkpoint.notify_all();
    });

    std::unique_lock<decltype(checkpoint_mutex)> lock(checkpoint_mutex);
    EXPECT_FALSE(checkpoint.wait_for(lock, std::chrono::milliseconds{50}, [&hit_checkpoint]() { return hit_checkpoint; }));
}

TEST(AsioMainLoopTest, alarm_fires_with_correct_delay)
{
    mir::AsioMainLoop ml;

    mtf::WatchDog runner([&ml]() {ml.stop();});

    auto alarm = ml.notify_in(std::chrono::milliseconds{50}, [&runner]()
    {
        runner.notify_done();
    });

    runner.run([&ml](mtf::WatchDog&) { ml.run(); });

    EXPECT_TRUE(runner.wait_for(std::chrono::milliseconds{100}));
}

TEST(AsioMainLoopTest, multiple_alarms_fire)
{
    mir::AsioMainLoop ml;

    mtf::WatchDog runner([&ml]() {ml.stop();});

    int const alarm_count{10};
    std::atomic<int> call_count{0};
    std::array<std::unique_ptr<mir::time::Alarm>, alarm_count> alarms;

    for (auto& alarm : alarms)
    {
        alarm = ml.notify_in(std::chrono::milliseconds{50}, [&runner, &call_count]()
        {
            call_count.fetch_add(1);
            if (call_count == alarm_count)
                runner.notify_done();
        });
    }

    runner.run([&ml](mtf::WatchDog&) { ml.run(); });

    EXPECT_TRUE(runner.wait_for(std::chrono::milliseconds{100}));
    for (auto& alarm : alarms)
        EXPECT_EQ(mir::time::Alarm::Triggered, alarm->state());
}


TEST(AsioMainLoopTest, alarm_changes_to_triggered_state)
{
    mir::AsioMainLoop ml;

    mtf::WatchDog runner([&ml]() {ml.stop();});

    auto alarm = ml.notify_in(std::chrono::milliseconds{50}, [&runner]()
    {
        runner.notify_done();
    });

    runner.run([&ml](mtf::WatchDog&) { ml.run(); });

    ASSERT_TRUE(runner.wait_for(std::chrono::milliseconds{100}));

    EXPECT_EQ(mir::time::Alarm::Triggered, alarm->state());
}

TEST(AsioMainLoopTest, alarm_starts_in_pending_state)
{
    mir::AsioMainLoop ml;

    mtf::WatchDog runner([&ml]() {ml.stop();});

    auto alarm = ml.notify_in(std::chrono::milliseconds{50}, [&runner]()
    {
        runner.notify_done();
    });

    runner.run([&ml](mtf::WatchDog&) { ml.run(); });

    EXPECT_EQ(mir::time::Alarm::Pending, alarm->state());
}

TEST(AsioMainLoopTest, cancelled_alarm_doesnt_fire)
{
    mir::AsioMainLoop ml;

    mtf::WatchDog runner([&ml]() {ml.stop();});

    auto alarm = ml.notify_in(std::chrono::milliseconds{200}, [&runner]()
    {
        runner.notify_done();
    });

    runner.run([&ml](mtf::WatchDog&) { ml.run(); });

    EXPECT_TRUE(alarm->cancel());
    EXPECT_FALSE(runner.wait_for(std::chrono::milliseconds{300}));
    EXPECT_EQ(mir::time::Alarm::Cancelled, alarm->state());
}

TEST(AsioMainLoopTest, destroyed_alarm_doesnt_fire)
{
    mir::AsioMainLoop ml;

    mtf::WatchDog runner([&ml]() {ml.stop();});

    auto alarm = ml.notify_in(std::chrono::milliseconds{200}, [&runner]()
    {
        runner.notify_done();
    });

    runner.run([&ml](mtf::WatchDog&) { ml.run(); });

    alarm.reset(nullptr);

    EXPECT_FALSE(runner.wait_for(std::chrono::milliseconds{300}));
}

TEST(AsioMainLoopTest, rescheduled_alarm_fires_again)
{
    mir::AsioMainLoop ml;

    // Somewhat counterintuitively we need some work on the mainloop to keep it running.
    auto dummy_alarm = ml.notify_in(std::chrono::milliseconds{1000}, [](){});

    mtf::WatchDog runner([&ml]() {ml.stop();});

    std::mutex m;
    std::condition_variable called;
    int call_count{0};

    auto alarm = ml.notify_in(std::chrono::milliseconds{0}, [&runner, &call_count, &m, &called]()
    {
        std::unique_lock<decltype(m)> lock(m);
        call_count++;
        if (call_count == 2)
            runner.notify_done();
        called.notify_all();
    });

    runner.run([&ml](mtf::WatchDog&) { ml.run(); });

    {
        std::unique_lock<decltype(m)> lock(m);
        ASSERT_TRUE(called.wait_for(lock,
                                    std::chrono::milliseconds{50},
                                    [&call_count](){ return call_count == 1; }));
    }

    ASSERT_EQ(mir::time::Alarm::Triggered, alarm->state());
    alarm->reschedule_in(std::chrono::milliseconds{100});
    EXPECT_EQ(mir::time::Alarm::Pending, alarm->state());

    EXPECT_TRUE(runner.wait_for(std::chrono::milliseconds{500}));
    EXPECT_EQ(mir::time::Alarm::Triggered, alarm->state());
}

TEST(AsioMainLoopTest, rescheduled_alarm_cancels_previous_scheduling)
{
    mir::AsioMainLoop ml;

    // Somewhat counterintuitively we need some work on the mainloop to keep it running.
    auto dummy_alarm = ml.notify_in(std::chrono::milliseconds{1000}, [](){});

    mtf::WatchDog runner([&ml]() {ml.stop();});

    int call_count{0};

    auto alarm = ml.notify_in(std::chrono::milliseconds{100}, [&runner, &call_count]()
    {
        call_count++;
        runner.notify_done();
    });

    runner.run([&ml](mtf::WatchDog&) { ml.run(); });

    EXPECT_TRUE(alarm->reschedule_in(std::chrono::milliseconds{150}));
    EXPECT_EQ(mir::time::Alarm::Pending, alarm->state());

    EXPECT_TRUE(runner.wait_for(std::chrono::milliseconds{500}));
    EXPECT_EQ(mir::time::Alarm::Triggered, alarm->state());
    EXPECT_EQ(1, call_count);
}

TEST(AsioMainLoopTest, alarm_fires_at_correct_time_point)
{
    mir::AsioMainLoop ml;
    mir::time::HighResolutionClock clock;

    mtf::WatchDog runner([&ml]() {ml.stop();});

    mir::time::Timestamp real_soon = clock.sample() + std::chrono::microseconds{120};

    auto alarm = ml.notify_at(real_soon, [&runner]()
    {
        runner.notify_done();
    });

    runner.run([&ml](mtf::WatchDog&) { ml.run(); });

    EXPECT_TRUE(runner.wait_for(std::chrono::milliseconds{200}));
}

