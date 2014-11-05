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

/*****************
 * GSourceHandle *
 *****************/

md::GSourceHandle::GSourceHandle(GSource* gsource)
    : gsource{gsource}, detach_on_destruction{false}
{
}

md::GSourceHandle::GSourceHandle(GSourceHandle&& other)
    : gsource{other.gsource}, detach_on_destruction{other.detach_on_destruction}
{
    other.gsource = nullptr;
}

md::GSourceHandle::~GSourceHandle()
{
    if (gsource)
    {
        if (detach_on_destruction)
            g_source_destroy(gsource);

        g_source_unref(gsource);
    }
}

md::GSourceHandle::operator GSource*() const
{
    return gsource;
}

void md::GSourceHandle::attach(GMainContext* main_context) const
{
    g_source_attach(gsource, main_context);
}

void md::GSourceHandle::attach_and_detach_on_destruction(GMainContext* main_context)
{
    detach_on_destruction = true;
    g_source_attach(gsource, main_context);
}

/******************
 * make_*_gsource *
 ******************/

md::GSourceHandle md::make_idle_gsource(int priority, std::function<void()> const& callback)
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

    GSourceHandle gsource{g_idle_source_new()};

    g_source_set_priority(gsource, priority);
    g_source_set_callback(
        gsource,
        reinterpret_cast<GSourceFunc>(&IdleContext::static_call),
        new IdleContext{callback},
        reinterpret_cast<GDestroyNotify>(&IdleContext::static_destroy));

    return gsource;
}

md::GSourceHandle md::make_signal_gsource(
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

    GSourceHandle gsource{g_unix_signal_source_new(sig)};

    g_source_set_callback(
        gsource,
        reinterpret_cast<GSourceFunc>(&SignalContext::static_call),
        new SignalContext{callback, sig},
        reinterpret_cast<GDestroyNotify>(&SignalContext::static_destroy));

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
    ~FdSource()
    {
        // g_source_destroy() may be called while the source is about to be
        // dispatched, so there is no guarantee that after g_source_destroy()
        // returns any associated handlers won't be called. Since we want a
        // stronger no-call guarantee we need to disable the callback manually
        // before destruction.
        if (ctx)
            ctx->disable_callback();
    }

    GSourceHandle gsource;
    FdContext* const ctx;
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
    GSourceHandle gsource{g_unix_fd_source_new(fd, G_IO_IN)};

    auto const fd_context = new FdContext{handler};

    g_source_set_callback(
        gsource,
        reinterpret_cast<GSourceFunc>(&FdContext::static_call),
        fd_context,
        reinterpret_cast<GDestroyNotify>(&FdContext::static_destroy));

    std::lock_guard<std::mutex> lock{sources_mutex};

    sources.emplace_back(new FdSource{std::move(gsource), fd_context, owner});
    sources.back()->gsource.attach_and_detach_on_destruction(main_context);
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
