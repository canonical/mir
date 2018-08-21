/*
 * Copyright © 2014-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
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

#ifndef MIR_GLIB_MAIN_LOOP_H_
#define MIR_GLIB_MAIN_LOOP_H_

#include "mir/main_loop.h"
#include "mir/glib_main_loop_sources.h"

#include <atomic>
#include <vector>
#include <mutex>
#include <exception>
#include <future>

#include <glib.h>
#include <deque>

namespace mir
{

namespace detail
{

class GMainContextHandle
{
public:
    GMainContextHandle();
    ~GMainContextHandle();
    operator GMainContext*() const;
private:
    GMainContext* const main_context;
};

}

class GLibMainLoop : public MainLoop
{
public:
    GLibMainLoop(std::shared_ptr<time::Clock> const& clock);

    void run() override;
    void stop() override;
    bool running() const override;

    void register_signal_handler(
        std::initializer_list<int> signals,
        std::function<void(int)> const& handler) override;

    void register_signal_handler(
        std::initializer_list<int> signals,
        mir::UniqueModulePtr<std::function<void(int)>> handler) override;

    void register_fd_handler(
        std::initializer_list<int> fds,
        void const* owner,
        std::function<void(int)> const& handler) override;

    void register_fd_handler(
        std::initializer_list<int> fds,
        void const* owner,
        mir::UniqueModulePtr<std::function<void(int)>> handler) override;

    void unregister_fd_handler(void const* owner) override;

    void enqueue(void const* owner, ServerAction const& action) override;
    void enqueue_with_guaranteed_execution(ServerAction const& action) override;

    void pause_processing_for(void const* owner) override;
    void resume_processing_for(void const* owner) override;

    std::unique_ptr<mir::time::Alarm> create_alarm(
        std::function<void()> const& callback) override;

    std::unique_ptr<mir::time::Alarm> create_alarm(
        std::unique_ptr<LockableCallback> callback) override;

    void spawn(std::function<void()>&& work) override;

    void reprocess_all_sources();

    /**
     * Run a functor with the GLibMainLoop's GMainContext as the thread-default context
     *
     * For a Callable with return type T this returns a std::future<T> with a destructor that
     * will not block on the shared state (ie: this is guaranteed not to be a std::async future)
     *
     * \param [in]  code    Functor to run; this may be any type invokable with no arguments.
     */
    template<
        typename Callable,
        typename =
        std::enable_if_t<!std::is_same<std::result_of_t<Callable()>, void>::value>
    >
    std::future<std::result_of_t<Callable()>> run_with_context_as_thread_default(Callable code)
    {
        using Result = std::result_of_t<Callable()>;

        auto promise = std::make_shared<std::promise<Result>>();
        auto future = promise->get_future();

        execute_with_context_as_thread_default(
            [code = std::move(code), promise = std::move(promise)]()
            {
                try
                {
                    promise->set_value(code());
                }
                catch(...)
                {
                    promise->set_exception(std::current_exception());
                }
            });

        return future;
    }

    template<
        typename Callable,
        typename =
            std::enable_if_t<std::is_same<std::result_of_t<Callable()>, void>::value>
        >
    std::future<void> run_with_context_as_thread_default(Callable code)
    {
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        execute_with_context_as_thread_default(
            [code = std::move(code), promise = std::move(promise)]()
            {
                try
                {
                    code();
                    promise->set_value();
                }
                catch(...)
                {
                    promise->set_exception(std::current_exception());
                }
            });

        return future;
    }
private:
    void execute_with_context_as_thread_default(std::function<void()> code);

    bool should_process_actions_for(void const* owner);
    void handle_exception(std::exception_ptr const& e);

    std::shared_ptr<time::Clock> const clock;
    detail::GMainContextHandle const main_context;
    std::atomic<bool> running_;
    detail::FdSources fd_sources;
    detail::SignalSources signal_sources;
    std::mutex do_not_process_mutex;
    std::vector<void const*> do_not_process;
    std::mutex run_on_halt_mutex;
    std::deque<ServerAction> run_on_halt_queue;
    std::function<void()> before_iteration_hook;
    std::exception_ptr main_loop_exception;
};

}

#endif
