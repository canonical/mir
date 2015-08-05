/*
 * Copyright © 2014-2015 Canonical Ltd.
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
 *              Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include "mir/glib_main_loop.h"
#include "mir/time/steady_clock.h"

#include "mir/test/signal.h"
#include "mir/test/pipe.h"
#include "mir/test/fake_shared.h"
#include "mir/test/auto_unblock_thread.h"
#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/doubles/mock_lockable_callback.h"
#include "mir_test_framework/process.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>

namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

template <typename T>
std::vector<T> values_from_to(T f, T t)
{
    std::vector<T> v;
    for (T i = f; i <= t; ++i)
        v.push_back(i);
    return v;
}

struct OnScopeExit
{
    ~OnScopeExit() { f(); }
    std::function<void()> const f;
};

void execute_in_forked_process(
    testing::Test* test,
    std::function<void()> const& f,
    std::function<void()> const& cleanup)
{
    auto const pid = fork();

    if (!pid)
    {
        {
            OnScopeExit on_scope_exit{cleanup};
            f();
        }
        exit(test->HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
    }
    else
    {
        mir_test_framework::Process child{pid};
        // Note: valgrind on armhf can very slow when dealing with forks,
        // so give it enough time.
        auto const result = child.wait_for_termination(std::chrono::seconds{30});
        EXPECT_TRUE(result.succeeded());
    }
}

struct GLibMainLoopTest : ::testing::Test
{
    mir::GLibMainLoop ml{std::make_shared<mir::time::SteadyClock>()};
    std::function<void()> const destroy_glib_main_loop{[this]{ ml.~GLibMainLoop(); }};
};

}

TEST_F(GLibMainLoopTest, stops_from_within_handler)
{
    mt::Signal loop_finished;

    mt::AutoJoinThread t{
        [&]
        {
            int const owner{0};
            ml.enqueue(&owner, [&] { ml.stop(); });
            ml.run();
            loop_finished.raise();
        }};

    EXPECT_TRUE(loop_finished.wait_for(std::chrono::seconds{5}));
}

TEST_F(GLibMainLoopTest, stops_from_outside_handler)
{
    mt::Signal loop_running;
    mt::Signal loop_finished;

    mt::AutoJoinThread t{
        [&]
        {
            int const owner{0};
            ml.enqueue(&owner, [&] { loop_running.raise(); });
            ml.run();
            loop_finished.raise();
        }};

    ASSERT_TRUE(loop_running.wait_for(std::chrono::seconds{5}));

    ml.stop();

    EXPECT_TRUE(loop_finished.wait_for(std::chrono::seconds{30}));
}

TEST_F(GLibMainLoopTest, ignores_handler_added_after_stop)
{
    int const owner{0};
    bool handler_called{false};
    mt::Signal loop_running;

    std::thread t{
        [&]
        {
            loop_running.wait();
            ml.stop();
            int const owner1{0};
            ml.enqueue(&owner1, [&] { handler_called = true; });
        }};

    ml.enqueue(&owner, [&] { loop_running.raise(); });
    ml.run();

    t.join();

    EXPECT_FALSE(handler_called);
}

TEST_F(GLibMainLoopTest, handles_signal)
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

TEST_F(GLibMainLoopTest, handles_multiple_signals)
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
        EXPECT_EQ(signals[i % signals.size()], handled_signals[i]) << " index " << i;
}

TEST_F(GLibMainLoopTest, invokes_all_registered_handlers_for_signal)
{
    using namespace testing;

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

    ASSERT_THAT(handled_signum, Each(signum));
}

TEST_F(GLibMainLoopTest, propagates_exception_from_signal_handler)
{
    // Execute in forked process to work around
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61643
    // causing subsequent tests to fail (e.g. MultiThreadedCompositor.*).
    execute_in_forked_process(this,
        [&]
        {
            int const signum{SIGUSR1};
            ml.register_signal_handler(
                {signum},
                [&] (int) { throw std::runtime_error("signal handler error"); });

            kill(getpid(), signum);

            EXPECT_THROW({ ml.run(); }, std::runtime_error);
        },
        // Since we terminate the forked process with an exit() call, objects on
        // the stack are not destroyed. We need to manually destroy the
        // GLibMainLoop object to avoid fd leaks.
        destroy_glib_main_loop);
}

TEST_F(GLibMainLoopTest, handles_fd)
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

TEST_F(GLibMainLoopTest, multiple_fds_with_single_handler_handled)
{
    using namespace testing;

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
    for (size_t i = 0; i < num_elems_to_send; i++)
        EXPECT_EQ(pipes[i % pipes.size()].read_fd(), handled_fds[i]) << " index " << i;

    EXPECT_THAT(elems_read, ContainerEq(values_from_to<size_t>(0, num_elems_to_send - 1)));
}

TEST_F(GLibMainLoopTest, multiple_fd_handlers_are_called)
{
    using namespace testing;

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

    EXPECT_THAT(handled_fds,
                ElementsAre(
                    pipes[0].read_fd(),
                    pipes[1].read_fd(),
                    pipes[2].read_fd()));

    EXPECT_THAT(elems_read, ContainerEq(elems_to_send));
}

TEST_F(GLibMainLoopTest,
       unregister_prevents_callback_and_does_not_harm_other_callbacks)
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

TEST_F(GLibMainLoopTest, unregister_does_not_close_fds)
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

TEST_F(GLibMainLoopTest, propagates_exception_from_fd_handler)
{
    // Execute in forked process to work around
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61643
    // causing subsequent tests to fail (e.g. MultiThreadedCompositor.*)
    execute_in_forked_process(this,
        [&]
        {
            mt::Pipe p;
            char const data_to_write{'a'};

            ml.register_fd_handler(
                {p.read_fd()},
                this,
                [] (int) { throw std::runtime_error("fd handler error"); });

            EXPECT_EQ(1, write(p.write_fd(), &data_to_write, 1));

            EXPECT_THROW({ ml.run(); }, std::runtime_error);
        },
        // Since we terminate the forked process with an exit() call, objects on
        // the stack are not destroyed. We need to manually destroy the
        // GLibMainLoop object to avoid fd leaks.
        destroy_glib_main_loop);
}

TEST_F(GLibMainLoopTest, can_unregister_fd_from_within_fd_handler)
{
    mt::Pipe p1;

    ml.register_fd_handler(
        {p1.read_fd()},
        this,
        [this](int)
        {
            ml.unregister_fd_handler(this);
            ml.stop();
        });

    EXPECT_EQ(1, write(p1.write_fd(), "a", 1));

    ml.run();
}

TEST_F(GLibMainLoopTest, dispatches_action)
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

TEST_F(GLibMainLoopTest, dispatches_multiple_actions_in_order)
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

    EXPECT_THAT(actions, ContainerEq(values_from_to(0, num_actions - 1)));
}

TEST_F(GLibMainLoopTest, does_not_dispatch_paused_actions)
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

    EXPECT_THAT(actions, ElementsAre(1, 3));
}

TEST_F(GLibMainLoopTest, dispatches_actions_resumed_from_within_another_action)
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

    EXPECT_THAT(actions, ElementsAre(1, 0));
}

TEST_F(GLibMainLoopTest, handles_enqueue_from_within_action)
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

    EXPECT_THAT(actions, ContainerEq(values_from_to(0, num_actions - 1)));
}

TEST_F(GLibMainLoopTest, dispatches_actions_resumed_externally)
{
    using namespace testing;

    std::vector<int> actions;
    void const* const owner1_ptr{&actions};
    int const owner2{0};
    mt::Signal action_with_id_1_done;

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
            action_with_id_1_done.raise();
        });

    ml.pause_processing_for(owner1_ptr);

    std::thread t{
        [&]
        {
            action_with_id_1_done.wait_for(std::chrono::seconds{5});
            ml.resume_processing_for(owner1_ptr);
        }};

    ml.run();

    t.join();

    EXPECT_TRUE(action_with_id_1_done.raised());
    EXPECT_THAT(actions, ElementsAre(1, 0));
}

TEST_F(GLibMainLoopTest, propagates_exception_from_server_action)
{
    // Execute in forked process to work around
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61643
    // causing subsequent tests to fail (e.g. MultiThreadedCompositor.*)
    execute_in_forked_process(this,
        [&]
        {
            ml.enqueue(this, [] { throw std::runtime_error("server action error"); });

            EXPECT_THROW({ ml.run(); }, std::runtime_error);
        },
        // Since we terminate the forked process with an exit() call, objects on
        // the stack are not destroyed. We need to manually destroy the
        // GLibMainLoop object to avoid fd leaks.
        destroy_glib_main_loop);
}

TEST_F(GLibMainLoopTest, can_be_rerun_after_exception)
{
    // Execute in forked process to work around
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61643
    // causing subsequent tests to fail (e.g. MultiThreadedCompositor.*)
    execute_in_forked_process(this,
        [&]
        {
            ml.enqueue(this, [] { throw std::runtime_error("server action error"); });

            EXPECT_THROW({
                ml.run();
            }, std::runtime_error);

            ml.enqueue(this, [&] { ml.stop(); });
            ml.run();
        },
        // Since we terminate the forked process with an exit() call, objects on
        // the stack are not destroyed. We need to manually destroy the
        // GLibMainLoop object to avoid fd leaks.
        destroy_glib_main_loop);
}

namespace
{

struct AdvanceableClock : mtd::AdvanceableClock
{
    void advance_by(std::chrono::milliseconds const step, mir::GLibMainLoop& ml)
    {
        mtd::AdvanceableClock::advance_by(step);
        ml.reprocess_all_sources();
    }
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

struct UnblockMainLoop : mt::AutoUnblockThread
{
    UnblockMainLoop(mir::GLibMainLoop& loop)
        : mt::AutoUnblockThread([&loop]() {loop.stop();},
                                [&loop]() {loop.run();})
    {}
};

struct GLibMainLoopAlarmTest : ::testing::Test
{
    std::shared_ptr<AdvanceableClock> clock = std::make_shared<AdvanceableClock>();
    mir::GLibMainLoop ml{clock};
    std::chrono::milliseconds delay{50};
    std::function<void()> const destroy_glib_main_loop{[this]{ ml.~GLibMainLoop(); }};
};

}

TEST_F(GLibMainLoopAlarmTest, main_loop_runs_until_stop_called)
{
    auto mainloop_started = std::make_shared<mt::Signal>();

    auto fire_on_mainloop_start = ml.create_alarm([mainloop_started]()
    {
        mainloop_started->raise();
    });
    fire_on_mainloop_start->reschedule_in(std::chrono::milliseconds{0});

    UnblockMainLoop unblocker(ml);

    ASSERT_TRUE(mainloop_started->wait_for(std::chrono::milliseconds{100}));

    auto timer_fired = std::make_shared<mt::Signal>();
    auto alarm = ml.create_alarm([timer_fired]
    {
        timer_fired->raise();
    });
    alarm->reschedule_in(std::chrono::milliseconds{10});

    clock->advance_by(std::chrono::milliseconds{10}, ml);
    EXPECT_TRUE(timer_fired->wait_for(std::chrono::milliseconds{500}));

    ml.stop();

    // Main loop should be stopped now
    timer_fired = std::make_shared<mt::Signal>();
    auto should_not_fire = ml.create_alarm([timer_fired]()
    {
        timer_fired->raise();
    });
    should_not_fire->reschedule_in(std::chrono::milliseconds{0});

    EXPECT_FALSE(timer_fired->wait_for(std::chrono::milliseconds{10}));
}

TEST_F(GLibMainLoopAlarmTest, alarm_starts_in_pending_state)
{
    auto alarm = ml.create_alarm([]{});

    UnblockMainLoop unblocker(ml);

    EXPECT_EQ(mir::time::Alarm::cancelled, alarm->state());
}

TEST_F(GLibMainLoopAlarmTest, alarm_fires_with_correct_delay)
{
    UnblockMainLoop unblocker(ml);

    auto alarm = ml.create_alarm([]{});
    alarm->reschedule_in(delay);

    clock->advance_by(delay - std::chrono::milliseconds{1}, ml);
    EXPECT_EQ(mir::time::Alarm::pending, alarm->state());

    clock->advance_by(delay, ml);
    EXPECT_EQ(mir::time::Alarm::triggered, alarm->state());
}

TEST_F(GLibMainLoopAlarmTest, multiple_alarms_fire)
{
    using namespace testing;

    int const alarm_count{10};
    Counter call_count;
    std::array<std::unique_ptr<mir::time::Alarm>, alarm_count> alarms;

    for (auto& alarm : alarms)
    {
        alarm = ml.create_alarm([&call_count]{ ++call_count;});
        alarm->reschedule_in(delay);
    }

    UnblockMainLoop unblocker(ml);
    clock->advance_by(delay, ml);

    call_count.wait_for(delay, alarm_count);
    EXPECT_THAT(call_count, Eq(alarm_count));

    for (auto const& alarm : alarms)
        EXPECT_EQ(mir::time::Alarm::triggered, alarm->state());
}

TEST_F(GLibMainLoopAlarmTest, alarm_changes_to_triggered_state)
{
    auto alarm_fired = std::make_shared<mt::Signal>();
    auto alarm = ml.create_alarm([alarm_fired]()
    {
        alarm_fired->raise();
    });
    alarm->reschedule_in(std::chrono::milliseconds{5});

    UnblockMainLoop unblocker(ml);

    clock->advance_by(delay, ml);
    ASSERT_TRUE(alarm_fired->wait_for(std::chrono::milliseconds{100}));

    EXPECT_EQ(mir::time::Alarm::triggered, alarm->state());
}

TEST_F(GLibMainLoopAlarmTest, cancelled_alarm_doesnt_fire)
{
    UnblockMainLoop unblocker(ml);
    auto alarm = ml.create_alarm([]{ FAIL() << "Alarm handler of canceld alarm called"; });
    alarm->reschedule_in(std::chrono::milliseconds{100});

    EXPECT_TRUE(alarm->cancel());

    EXPECT_EQ(mir::time::Alarm::cancelled, alarm->state());

    clock->advance_by(std::chrono::milliseconds{100}, ml);

    EXPECT_EQ(mir::time::Alarm::cancelled, alarm->state());
}

TEST_F(GLibMainLoopAlarmTest, destroyed_alarm_doesnt_fire)
{
    auto alarm = ml.create_alarm([]{ FAIL() << "Alarm handler of destroyed alarm called"; });
    alarm->reschedule_in(std::chrono::milliseconds{200});

    UnblockMainLoop unblocker(ml);

    alarm.reset(nullptr);
    clock->advance_by(std::chrono::milliseconds{200}, ml);
}

TEST_F(GLibMainLoopAlarmTest, rescheduled_alarm_fires_again)
{
    std::atomic<int> call_count{0};

    auto alarm = ml.create_alarm([&call_count]()
    {
        if (call_count++ > 1)
            FAIL() << "Alarm called too many times";
    });
    alarm->reschedule_in(std::chrono::milliseconds{0});

    UnblockMainLoop unblocker(ml);

    clock->advance_by(std::chrono::milliseconds{0}, ml);
    ASSERT_EQ(mir::time::Alarm::triggered, alarm->state());

    alarm->reschedule_in(std::chrono::milliseconds{100});
    EXPECT_EQ(mir::time::Alarm::pending, alarm->state());

    clock->advance_by(std::chrono::milliseconds{100}, ml);
    EXPECT_EQ(mir::time::Alarm::triggered, alarm->state());
}

TEST_F(GLibMainLoopAlarmTest, rescheduled_alarm_cancels_previous_scheduling)
{
    std::atomic<int> call_count{0};

    auto alarm = ml.create_alarm([&call_count]()
    {
        call_count++;
    });
    alarm->reschedule_in(std::chrono::milliseconds{100});

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

TEST_F(GLibMainLoopAlarmTest, alarm_callback_preserves_lock_ordering)
{
    using namespace testing;

    mtd::MockLockableCallback handler;
    {
        InSequence s;
        EXPECT_CALL(handler, lock());
        EXPECT_CALL(handler, functor());
        EXPECT_CALL(handler, unlock());
    }

    auto alarm = ml.create_alarm(mt::fake_shared(handler));

    UnblockMainLoop unblocker(ml);
    alarm->reschedule_in(std::chrono::milliseconds{10});
    clock->advance_by(std::chrono::milliseconds{11}, ml);
}

TEST_F(GLibMainLoopAlarmTest, alarm_fires_at_correct_time_point)
{
    mir::time::Timestamp real_soon = clock->now() + std::chrono::milliseconds{120};

    auto alarm = ml.create_alarm([]{});
    alarm->reschedule_for(real_soon);

    UnblockMainLoop unblocker(ml);

    clock->advance_by(std::chrono::milliseconds{119}, ml);
    EXPECT_EQ(mir::time::Alarm::pending, alarm->state());

    clock->advance_by(std::chrono::milliseconds{1}, ml);
    EXPECT_EQ(mir::time::Alarm::triggered, alarm->state());
}

TEST_F(GLibMainLoopAlarmTest, propagates_exception_from_alarm)
{
    // Execute in forked process to work around
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61643
    // causing subsequent tests to fail (e.g. MultiThreadedCompositor.*).
    execute_in_forked_process(this,
        [&]
        {
            auto alarm = ml.create_alarm([] { throw std::runtime_error("alarm error"); });
            alarm->reschedule_in(std::chrono::milliseconds{0});

            EXPECT_THROW({ ml.run(); }, std::runtime_error);
        },
        // Since we terminate the forked process with an exit() call, objects on
        // the stack are not destroyed. We need to manually destroy the
        // GLibMainLoop object to avoid fd leaks.
        destroy_glib_main_loop);
}

TEST_F(GLibMainLoopAlarmTest, can_reschedule_alarm_from_within_alarm_callback)
{
    using namespace testing;

    int num_triggers = 0;
    int const expected_triggers = 3;

    std::shared_ptr<mir::time::Alarm> alarm = ml.create_alarm(
        [&]
        {
            if (++num_triggers == expected_triggers)
                ml.stop();
            else
                alarm->reschedule_in(std::chrono::milliseconds{0});
        });

    alarm->reschedule_in(std::chrono::milliseconds{0});

    ml.run();

    EXPECT_THAT(num_triggers, Eq(expected_triggers));
}

// More targeted regression test for LP: #1381925
TEST_F(GLibMainLoopTest, stress_emits_alarm_notification_with_zero_timeout)
{
    UnblockMainLoop unblocker{ml};

    for (int i = 0; i < 1000; ++i)
    {
        mt::Signal notification_called;

        auto alarm = ml.create_alarm([&]{ notification_called.raise(); });
        alarm->reschedule_in(std::chrono::milliseconds{0});

        EXPECT_TRUE(notification_called.wait_for(std::chrono::seconds{15}));
    }
}

// This test recreates a scenario we get in our integration and acceptance test
// runs, and which creates problems for the default glib signal source. The
// scenario involves creating, running (with signal handling) and destroying
// the main loop in the main process and then trying to do the same in a forked
// process. This happens, for example, when we run some tests with an in-process
// server setup followed by a test using an out-of-process server setup.
TEST(GLibMainLoopForkTest, handles_signals_when_created_in_forked_process)
{
    auto const check_mainloop_signal_handling =
        []
        {
            int const signum = SIGUSR1;
            mir::GLibMainLoop ml{std::make_shared<mir::time::SteadyClock>()};
            ml.register_signal_handler(
                {signum},
                [&](int)
                {
                   ml.stop();
                });

            kill(getpid(), signum);

            ml.run();
        };

    check_mainloop_signal_handling();

    // In problematic implementations the main loop in the forked process
    // doesn't handle signals, and thus hangs forever.
    auto const no_cleanup = []{};
    execute_in_forked_process(this, [&] { check_mainloop_signal_handling(); }, no_cleanup);
}
