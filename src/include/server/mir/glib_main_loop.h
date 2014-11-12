/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#include <glib.h>

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

class GLibMainLoop
{
public:
    GLibMainLoop(std::shared_ptr<time::Clock> const& clock);

    void run();
    void stop();

    void register_signal_handler(
        std::initializer_list<int> signals,
        std::function<void(int)> const& handler);

    void register_fd_handler(
        std::initializer_list<int> fds,
        void const* owner,
        std::function<void(int)> const& handler);

    void unregister_fd_handler(void const* owner);

    void enqueue(void const* owner, ServerAction const& action);
    void pause_processing_for(void const* owner);
    void resume_processing_for(void const* owner);

    std::unique_ptr<mir::time::Alarm> notify_in(
        std::chrono::milliseconds delay,
        std::function<void()> callback);

    std::unique_ptr<mir::time::Alarm> notify_at(
        mir::time::Timestamp t,
        std::function<void()> callback);

    std::unique_ptr<mir::time::Alarm> create_alarm(
        std::function<void()> callback);

    void reprocess_all_sources();

private:
    bool should_process_actions_for(void const* owner);

    std::shared_ptr<time::Clock> const clock;
    detail::GMainContextHandle const main_context;
    std::atomic<bool> running;
    detail::FdSources fd_sources;
    detail::SignalSources signal_sources;
    std::mutex do_not_process_mutex;
    std::vector<void const*> do_not_process;
    std::function<void()> before_iteration_hook;
};

}

#endif
