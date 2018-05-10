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
 *              Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include "mir/glib_main_loop_sources.h"
#include "mir/lockable_callback.h"
#include "mir/raii.h"

#include <algorithm>
#include <atomic>
#include <system_error>
#include <sstream>

#include <csignal>
#include <unistd.h>
#include <fcntl.h>

#include <boost/throw_exception.hpp>
#include <glib-unix.h>

namespace md = mir::detail;

namespace
{

class GSourceRef
{
public:
    GSourceRef(GSource* gsource) : gsource{gsource} {}
    ~GSourceRef() { if (gsource) g_source_unref(gsource); }
    operator GSource*() const { return gsource; }

private:
    GSourceRef(GSourceRef const&) = delete;
    GSourceRef& operator=(GSourceRef const&) = delete;

    GSource* gsource;
};

#ifndef GLIB_HAS_FIXED_LP_1401488
gboolean idle_callback(gpointer)
{
    return G_SOURCE_REMOVE; // Remove the idle source. Only needed once.
}

void destroy_idler(gpointer user_data)
{
    // idle_callback is never totally guaranteed to be called. However this
    // function is, so we do the unref here...
    auto gsource = static_cast<GSource*>(user_data);
    g_source_unref(gsource);
}
#endif

}

/*****************
 * GSourceHandle *
 *****************/

md::GSourceHandle::GSourceHandle()
    : GSourceHandle(nullptr, [](GSource*){})
{
}

md::GSourceHandle::GSourceHandle(
    GSource* gsource,
    std::function<void(GSource*)> const& terminate_dispatch)
    : gsource(gsource),
      terminate_dispatch(terminate_dispatch)
{
}

md::GSourceHandle::GSourceHandle(GSourceHandle&& other)
    : gsource(std::move(other.gsource)),
      terminate_dispatch(std::move(other.terminate_dispatch))
{
    other.gsource = nullptr;
    other.terminate_dispatch = [](GSource*){};
}

md::GSourceHandle& md::GSourceHandle::operator=(GSourceHandle other)
{
    std::swap(other.gsource, gsource);
    std::swap(other.terminate_dispatch, terminate_dispatch);
    return *this;
}

md::GSourceHandle::~GSourceHandle()
{
    if (gsource)
    {
#ifdef GLIB_HAS_FIXED_LP_1401488
        g_source_destroy(gsource);
        g_source_unref(gsource);
#else
        /*
         * https://bugs.launchpad.net/mir/+bug/1401488
         * We would like to g_source_unref(gsource); here but it's unsafe
         * to do so. The reason being we could be in some arbitrary thread,
         * and so the main loop might be mid-iteration of the same source
         * making callbacks. And glib lacks protection to prevent sources
         * getting fully free()'d mid-callback (TODO: fix glib?). So we defer
         * the final unref of the source to a point in the loop when it's safe.
         */
        if (!g_source_is_destroyed(gsource))
        {
            auto main_context = g_source_get_context(gsource);
            g_source_destroy(gsource);
            auto idler = g_idle_source_new();
            g_source_set_callback(idler, idle_callback, gsource, destroy_idler);
            g_source_attach(idler, main_context);
            g_source_unref(idler);
        }
        else
        {
            // The source is already destroyed so it's safe to unref now
            g_source_unref(gsource);
        }
#endif
    }
}

void md::GSourceHandle::ensure_no_further_dispatch()
{
    if (gsource)
    {
        terminate_dispatch(gsource);
    }
}

md::GSourceHandle::operator GSource*() const
{
    return gsource;
}

/*****************
 * add_*_gsource *
 *****************/

void md::add_idle_gsource(
    GMainContext* main_context, int priority, std::function<void()> const& callback)
{
    struct IdleContext
    {
        static gboolean static_call(IdleContext* ctx)
        {
            ctx->callback();
            return G_SOURCE_REMOVE;
        }
        static void static_destroy(IdleContext* ctx) { delete ctx; }
        std::function<void()> const callback;
    };

    GSourceRef gsource{g_idle_source_new()};

    g_source_set_priority(gsource, priority);
    g_source_set_callback(
        gsource,
        reinterpret_cast<GSourceFunc>(&IdleContext::static_call),
        new IdleContext{callback},
        reinterpret_cast<GDestroyNotify>(&IdleContext::static_destroy));

    g_source_attach(gsource, main_context);
}

void md::add_server_action_gsource(
    GMainContext* main_context,
    void const* owner,
    std::function<void()> const& action,
    std::function<bool(void const*)> const& should_dispatch)
{
    struct ServerActionContext
    {
        void const* const owner;
        std::function<void(void)> const action;
        std::function<bool(void const*)> const should_dispatch;

        // If we come to finalize() before dispatch() we have already
        // torn down most of Mir and even unloaded some shared libraries.
        // That means the action could refer to  stuff that is no longer
        // in the address space.
        // We will just leak any resources instead of crashing.
        bool mutable dispatched{false};
    };

    struct ServerActionGSource
    {
        GSource gsource;
        ServerActionContext ctx;
        bool ctx_constructed;

        static gboolean prepare(GSource* source, gint *timeout)
        {
            *timeout = -1;
            auto const& ctx = reinterpret_cast<ServerActionGSource*>(source)->ctx;
            return ctx.should_dispatch(ctx.owner);
        }

        static gboolean check(GSource* source)
        {
            auto const& ctx = reinterpret_cast<ServerActionGSource*>(source)->ctx;
            return ctx.should_dispatch(ctx.owner);
        }

        static gboolean dispatch(GSource* source, GSourceFunc, gpointer)
        {
            auto const& ctx = reinterpret_cast<ServerActionGSource*>(source)->ctx;
            ctx.action();
            ctx.dispatched = true;
            return FALSE;
        }

        static void finalize(GSource* source)
        {
            auto const sa_gsource = reinterpret_cast<ServerActionGSource*>(source);
            if (sa_gsource->ctx_constructed && sa_gsource->ctx.dispatched)
                sa_gsource->ctx.~ServerActionContext();
        }
    };

    static GSourceFuncs gsource_funcs{
        ServerActionGSource::prepare,
        ServerActionGSource::check,
        ServerActionGSource::dispatch,
        ServerActionGSource::finalize,
        nullptr,
        nullptr
    };

    GSourceRef gsource{g_source_new(&gsource_funcs, sizeof(ServerActionGSource))};
    auto const sa_gsource = reinterpret_cast<ServerActionGSource*>(static_cast<GSource*>(gsource));

    sa_gsource->ctx_constructed = false;
    new (&sa_gsource->ctx) decltype(sa_gsource->ctx){owner, action, should_dispatch};
    sa_gsource->ctx_constructed = true;

    g_source_attach(gsource, main_context);
}

md::GSourceHandle md::add_timer_gsource(
    GMainContext* main_context,
    std::shared_ptr<time::Clock> const& clock,
    std::shared_ptr<LockableCallback> const& handler,
    std::function<void()> const& exception_handler,
    time::Timestamp target_time)
{
    struct TimerContext
    {
        TimerContext(std::shared_ptr<time::Clock> const& clock,
                     std::shared_ptr<LockableCallback> const& handler,
                     std::function<void()> const& exception_handler,
                     time::Timestamp target_time)
            : clock{clock}, handler{handler}, exception_handler{exception_handler},
              target_time{target_time}, enabled{true}
        {
        }
        std::shared_ptr<time::Clock> clock;
        std::shared_ptr<LockableCallback> handler;
        std::function<void()> exception_handler;
        time::Timestamp target_time;
        std::recursive_mutex mutex;
        bool enabled;
    };

    struct TimerGSource
    {
        GSource gsource;
        TimerContext ctx;
        bool ctx_constructed;

        static gboolean prepare(GSource* source, gint *timeout)
        {
            auto const& ctx = reinterpret_cast<TimerGSource*>(source)->ctx;

            auto const now = ctx.clock->now();
            bool const ready = (now >= ctx.target_time);
            if (ready)
                *timeout = -1;
            else
                *timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
                    ctx.clock->min_wait_until(ctx.target_time)).count();

            return ready;
        }

        static gboolean check(GSource* source)
        {
            auto const& ctx = reinterpret_cast<TimerGSource*>(source)->ctx;

            auto const now = ctx.clock->now();
            bool const ready = (now >= ctx.target_time);
            return ready;
        }

        static gboolean dispatch(GSource* source, GSourceFunc, gpointer)
        {
            auto& ctx = reinterpret_cast<TimerGSource*>(source)->ctx;
            try
            {
                // Attempt to preserve locking order during callback dispatching
                // so we acquire the caller's lock before our own.
                auto& handler = *ctx.handler;
                std::lock_guard<LockableCallback> handler_lock{handler};
                std::lock_guard<decltype(ctx.mutex)> lock{ctx.mutex};
                if (ctx.enabled)
                    handler();
            }
            catch(...)
            {
                ctx.exception_handler();
            }

            return FALSE;
        }

        static void finalize(GSource* source)
        {
            auto const timer_gsource = reinterpret_cast<TimerGSource*>(source);
            if (timer_gsource->ctx_constructed)
                timer_gsource->ctx.~TimerContext();
        }

        static void disable(GSource* source)
        {
            auto& ctx = reinterpret_cast<TimerGSource*>(source)->ctx;
            std::lock_guard<decltype(ctx.mutex)> lock{ctx.mutex};
            ctx.enabled = false;
        }
    };

    static GSourceFuncs gsource_funcs{
        TimerGSource::prepare,
        TimerGSource::check,
        TimerGSource::dispatch,
        TimerGSource::finalize,
        nullptr,
        nullptr
    };

    GSourceHandle gsource{
        g_source_new(&gsource_funcs, sizeof(TimerGSource)),
        [](GSource* gsource) { TimerGSource::disable(gsource); }};
    auto const timer_gsource = reinterpret_cast<TimerGSource*>(static_cast<GSource*>(gsource));

    timer_gsource->ctx_constructed = false;
    new (&timer_gsource->ctx) TimerContext{clock, handler, exception_handler, target_time};
    timer_gsource->ctx_constructed = true;

    g_source_attach(gsource, main_context);

    return gsource;
}

/*************
 * FdSources *
 *************/

struct md::FdSources::FdContext
{
    FdContext(std::function<void(int)> const& handler)
        : handler{handler}, enabled{true}
    {
    }

    void disable_callback()
    {
        std::lock_guard<decltype(mutex)> lock{mutex};
        enabled = false;
    }

    static gboolean static_call(int fd, GIOCondition, FdContext* ctx)
    {
        std::lock_guard<decltype(ctx->mutex)> lock{ctx->mutex};

        if (ctx->enabled)
            ctx->handler(fd);

        return G_SOURCE_CONTINUE;
    }

    static void static_destroy(FdContext* ctx)
    {
        delete ctx;
    }

private:
    std::function<void(int)> handler;
    bool enabled;
    std::recursive_mutex mutex;
};

struct md::FdSources::FdSource
{
    ~FdSource()
    {
        gsource.ensure_no_further_dispatch();
    }

    GSourceHandle gsource;
    void const* const owner;
};

md::FdSources::FdSources(GMainContext* main_context)
    : main_context{main_context}
{
}

md::FdSources::~FdSources() = default;

void md::FdSources::add(
    int fd, void const* owner, std::function<void(int)> const& handler)
{
    auto const fd_context = new FdContext{handler};

    // g_source_destroy() may be called while the source is about to be
    // dispatched, so there is no guarantee that after g_source_destroy()
    // returns any associated handlers won't be called. Since we want a
    // stronger guarantee we need to disable the callback manually before
    // destruction.
    GSourceHandle gsource{
        g_unix_fd_source_new(fd, G_IO_IN),
        [=] (GSource*) { fd_context->disable_callback(); }};

    g_source_set_callback(
        gsource,
        reinterpret_cast<GSourceFunc>(reinterpret_cast<void*>(&FdContext::static_call)),
        fd_context,
        reinterpret_cast<GDestroyNotify>(reinterpret_cast<void*>(&FdContext::static_destroy)));

    std::lock_guard<std::mutex> lock{sources_mutex};

    sources.emplace_back(new FdSource{std::move(gsource), owner});
    g_source_attach(sources.back()->gsource, main_context);
}

void md::FdSources::remove_all_owned_by(void const* owner)
{
    std::lock_guard<std::mutex> lock{sources_mutex};

    auto const new_end = std::remove_if(
        sources.begin(), sources.end(),
        [&] (std::unique_ptr<FdSource> const& fd_source)
        {
            return fd_source->owner == owner;
        });

    sources.erase(new_end, sources.end());
}

/*****************
 * SignalSources *
 *****************/

class md::SignalSources::SourceRegistration
{
public:
    SourceRegistration(int write_fd)
        : write_fd{write_fd}
    {
        init_write_fds();
        add_write_fd();
    }

    ~SourceRegistration()
    {
        remove_write_fd();
    }

    static void notify_sources_of_signal(int sig)
    {
        for (auto const& write_fd : write_fds)
        {
            // There is a benign race here: write_fd may have changed
            // between checking and using it. This doesn't matter
            // since in the worst case we will call write() with an invalid
            // fd (-1) which is harmless.
            if (write_fd >= 0 && write(write_fd, &sig, sizeof(sig))) {}
        }
    }

private:
    void init_write_fds()
    {
        static std::once_flag once;
        std::call_once(once,
            [&]
            {
                for (auto& wfd : write_fds)
                    wfd = -1;
            });
    }

    void add_write_fd()
    {
        for (auto& wfd : write_fds)
        {
            int v = -1;
            if (wfd.compare_exchange_strong(v, write_fd))
                return;
        }

        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Failed to add signal write fd. Have you created too many main loops?"));
    }

    void remove_write_fd()
    {
        for (auto& wfd : write_fds)
        {
            int v = write_fd;
            if (wfd.compare_exchange_strong(v, -1))
                break;
        }
    }

    static int const max_write_fds{10};
    static std::array<std::atomic<int>, max_write_fds> write_fds;
    int const write_fd;
};

std::array<std::atomic<int>,10> md::SignalSources::SourceRegistration::write_fds;

md::SignalSources::SignalSources(md::FdSources& fd_sources)
    : fd_sources(fd_sources)
{
    int pipefd[2];

    if (pipe(pipefd) == -1)
    {
        BOOST_THROW_EXCEPTION(
            std::system_error(errno, std::system_category(), "Failed to create signal pipe"));
    }

    signal_read_fd = mir::Fd(pipefd[0]);
    signal_write_fd = mir::Fd(pipefd[1]);

    fcntl(signal_read_fd, F_SETFD, FD_CLOEXEC);
    fcntl(signal_write_fd, F_SETFD, FD_CLOEXEC);
    // Make the signal_write_fd non-blocking, to avoid blocking in the signal handler
    fcntl(signal_write_fd, F_SETFL, O_NONBLOCK);

    source_registration.reset(new SourceRegistration{signal_write_fd});

    fd_sources.add(signal_read_fd, this,
        [this] (int) { dispatch_pending_signal(); });
}

md::SignalSources::~SignalSources()
{
    for (auto const& handled : handled_signals)
        sigaction(handled.first, &handled.second, nullptr);

    fd_sources.remove_all_owned_by(this);
}

void md::SignalSources::dispatch_pending_signal()
{
    auto const sig = read_pending_signal();
    if (sig != -1)
        dispatch_signal(sig);
}

int md::SignalSources::read_pending_signal()
{
    int sig = -1;
    size_t total = 0;

    do
    {
        auto const nread = read(
            signal_read_fd,
            reinterpret_cast<char*>(&sig) + total,
            sizeof(sig) - total);

        if (nread < 0)
        {
            if (errno != EINTR)
                return -1;
        }
        else
        {
            total += nread;
        }
    }
    while (total < sizeof(sig));

    return sig;
}

void md::SignalSources::add(
    std::vector<int> const& sigs, std::function<void(int)> const& handler)
{
    handlers.add({sigs, handler});
    for (auto sig : sigs)
        ensure_signal_is_handled(sig);
}

void md::SignalSources::ensure_signal_is_handled(int sig)
{
    std::lock_guard<std::mutex> lock{handled_signals_mutex};

    if (handled_signals.find(sig) != handled_signals.end())
        return;

    static int const no_flags{0};
    struct sigaction old_action;
    struct sigaction new_action;

    new_action.sa_handler = SourceRegistration::notify_sources_of_signal;
    sigfillset(&new_action.sa_mask);
    new_action.sa_flags = no_flags;

    if (sigaction(sig, &new_action, &old_action) == -1)
    {
        std::stringstream msg;
        msg << "Failed to register action for signal " << sig;
        BOOST_THROW_EXCEPTION(
            std::system_error(errno, std::system_category(), msg.str()));
    }

    handled_signals.emplace(sig, old_action);
}

void md::SignalSources::dispatch_signal(int sig)
{
    handlers.for_each(
        [&] (HandlerElement const& element)
        {
            if (std::find(element.sigs.begin(), element.sigs.end(), sig) != element.sigs.end())
                element.handler(sig);
        });
}
