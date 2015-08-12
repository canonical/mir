/*
 * Copyright © 2014-2015 Canonical Ltd.
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

#ifndef MIR_GLIB_MAIN_LOOP_SOURCES_H_
#define MIR_GLIB_MAIN_LOOP_SOURCES_H_

#include "mir/time/clock.h"
#include "mir/thread_safe_list.h"
#include "mir/fd.h"

#include <functional>
#include <vector>
#include <mutex>
#include <memory>
#include <unordered_map>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#endif

#include <glib.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace mir
{
class LockableCallback;
namespace detail
{

class GSourceHandle
{
public:
    GSourceHandle();
    GSourceHandle(GSource* gsource, std::function<void(GSource*)> const& terminate_dispatch);
    GSourceHandle(GSourceHandle&& other);
    GSourceHandle& operator=(GSourceHandle other);
    ~GSourceHandle();

    void ensure_no_further_dispatch();

    operator GSource*() const;

private:
    GSource* gsource;
    std::function<void(GSource*)> terminate_dispatch;
};

void add_idle_gsource(
    GMainContext* main_context, int priority, std::function<void()> const& callback);

void add_server_action_gsource(
    GMainContext* main_context,
    void const* owner,
    std::function<void()> const& action,
    std::function<bool(void const*)> const& should_dispatch);

GSourceHandle add_timer_gsource(
    GMainContext* main_context,
    std::shared_ptr<time::Clock> const& clock,
    std::shared_ptr<LockableCallback> const& handler,
    std::function<void()> const& exception_handler,
    time::Timestamp target_time);

class FdSources
{
public:
    FdSources(GMainContext* main_context);
    ~FdSources();

    void add(int fd, void const* owner, std::function<void(int)> const& handler);
    void remove_all_owned_by(void const* owner);

private:
    struct FdContext;
    struct FdSource;

    GMainContext* const main_context;
    std::mutex sources_mutex;
    std::vector<std::unique_ptr<FdSource>> sources;
};

class SignalSources
{
public:
    SignalSources(FdSources& fd_sources);
    ~SignalSources();

    void add(std::vector<int> const& sigs, std::function<void(int)> const& handler);

private:
    class SourceRegistration;
    struct HandlerElement
    {
        operator bool() const { return !!handler; }
        std::vector<int> sigs;
        std::function<void(int)> handler;
    };

    void dispatch_pending_signal();
    void ensure_signal_is_handled(int sig);
    int read_pending_signal();
    void dispatch_signal(int sig);

    FdSources& fd_sources;
    mir::Fd signal_read_fd;
    mir::Fd signal_write_fd;
    mir::ThreadSafeList<HandlerElement> handlers;
    std::mutex handled_signals_mutex;
    std::unordered_map<int, struct sigaction> handled_signals;
    std::unique_ptr<SourceRegistration> source_registration;
};

}
}

#endif
