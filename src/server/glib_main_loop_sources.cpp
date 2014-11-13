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

#include "mir/glib_main_loop_sources.h"
#include "mir/recursive_read_write_mutex.h"

#include <glib-unix.h>
#include <algorithm>

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
    std::function<void(GSource*)> const& pre_destruction_hook)
    : gsource{gsource},
      pre_destruction_hook{pre_destruction_hook}
{
}

md::GSourceHandle::GSourceHandle(GSourceHandle&& other)
    : gsource{std::move(other.gsource)},
      pre_destruction_hook{std::move(other.pre_destruction_hook)}
{
    other.gsource = nullptr;
    other.pre_destruction_hook = [](GSource*){};
}

md::GSourceHandle& md::GSourceHandle::operator=(GSourceHandle other)
{
    std::swap(other.gsource, gsource);
    std::swap(other.pre_destruction_hook, pre_destruction_hook);
    return *this;
}

md::GSourceHandle::~GSourceHandle()
{
    if (gsource)
    {
        pre_destruction_hook(gsource);
        g_source_destroy(gsource);
        g_source_unref(gsource);
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

void md::add_signal_gsource(
    GMainContext* main_context,
    int sig, std::function<void(int)> const& callback)
{
    struct SignalContext
    {
        static gboolean static_call(SignalContext* ctx)
        {
            ctx->callback(ctx->sig);
            return G_SOURCE_CONTINUE;
        }
        static void static_destroy(SignalContext* ctx) { delete ctx; }
        std::function<void(int)> const callback;
        int sig;
    };

    GSourceRef gsource{g_unix_signal_source_new(sig)};

    g_source_set_callback(
        gsource,
        reinterpret_cast<GSourceFunc>(&SignalContext::static_call),
        new SignalContext{callback, sig},
        reinterpret_cast<GDestroyNotify>(&SignalContext::static_destroy));

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
            return FALSE;
        }

        static void finalize(GSource* source)
        {
            auto const sa_gsource = reinterpret_cast<ServerActionGSource*>(source);
            if (sa_gsource->ctx_constructed)
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
    std::function<void()> const& handler,
    time::Timestamp target_time)
{
    struct TimerContext
    {
        TimerContext(std::shared_ptr<time::Clock> const& clock,
                     std::function<void()> const& handler,
                     time::Timestamp target_time)
            : clock{clock}, handler{handler}, target_time{target_time}, enabled{true}
        {
        }
        std::shared_ptr<time::Clock> clock;
        std::function<void()> handler;
        time::Timestamp target_time;
        bool enabled;
        mir::RecursiveReadWriteMutex mutex;
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

            RecursiveReadLock lock{ctx.mutex};
            if (ctx.enabled)
                ctx.handler();

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
            RecursiveWriteLock lock{ctx.mutex};
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
    new (&timer_gsource->ctx) TimerContext{clock, handler, target_time};
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
        RecursiveWriteLock lock{mutex};
        enabled = false;
    }

    static gboolean static_call(int fd, GIOCondition, FdContext* ctx)
    {
        RecursiveReadLock lock{ctx->mutex};

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
    mir::RecursiveReadWriteMutex mutex;
};

struct md::FdSources::FdSource
{
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
        reinterpret_cast<GSourceFunc>(&FdContext::static_call),
        fd_context,
        reinterpret_cast<GDestroyNotify>(&FdContext::static_destroy));

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
