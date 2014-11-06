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

#include <glib-unix.h>

namespace md = mir::detail;

/*****************
 * GSourceHandle *
 *****************/

md::GSourceHandle::GSourceHandle(GSource* gsource)
    : gsource{gsource}
{
}

md::GSourceHandle::GSourceHandle(md::GSourceHandle&& other)
    : gsource{other.gsource}
{
    other.gsource = nullptr;
}

md::GSourceHandle::~GSourceHandle()
{
    if (gsource)
        g_source_unref(gsource);
}

void md::GSourceHandle::attach(GMainContext* main_context) const
{
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

    auto const gsource = g_idle_source_new();
    g_source_set_priority(gsource, priority);
    g_source_set_callback(
        gsource,
        reinterpret_cast<GSourceFunc>(&IdleContext::static_call),
        new IdleContext{callback},
        reinterpret_cast<GDestroyNotify>(&IdleContext::static_destroy));

    return GSourceHandle{gsource};
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

    auto const gsource = g_unix_signal_source_new(sig);
    g_source_set_callback(
        gsource,
        reinterpret_cast<GSourceFunc>(&SignalContext::static_call),
        new SignalContext{callback, sig},
        reinterpret_cast<GDestroyNotify>(&SignalContext::static_destroy));

    return GSourceHandle{gsource};
}
