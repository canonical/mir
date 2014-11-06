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

#include "mir/glib_main_loop.h"

#include <functional>
#include <stdexcept>

#include <boost/throw_exception.hpp>

namespace
{

class GSourceHandle
{
public:
    explicit GSourceHandle(GSource* gsource)
        : gsource{gsource}
    {
    }

    GSourceHandle(GSourceHandle&& other)
        : gsource{other.gsource}
    {
        other.gsource = nullptr;
    }

    ~GSourceHandle()
    {
        if (gsource)
            g_source_unref(gsource);
    }

    void attach(GMainContext* main_context) const
    {
        g_source_attach(gsource, main_context);
    }

private:
    GSource* gsource;
};

GSourceHandle make_idle_source(int priority, std::function<void()> const& callback)
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

}

mir::detail::GMainContextHandle::GMainContextHandle()
    : main_context{g_main_context_new()}
{
    if (!main_context)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create GMainContext"));
}

mir::detail::GMainContextHandle::~GMainContextHandle()
{
    if (main_context)
        g_main_context_unref(main_context);
}

mir::detail::GMainContextHandle::operator GMainContext*() const
{
    return main_context;
}


mir::GLibMainLoop::GLibMainLoop()
    : running{false}
{
}

void mir::GLibMainLoop::run()
{
    running = true;

    while (running)
    {
        g_main_context_iteration(main_context, TRUE);
    }
}

void mir::GLibMainLoop::stop()
{
    auto const source = make_idle_source(G_PRIORITY_HIGH,
        [this]
        {
            running = false;
            g_main_context_wakeup(main_context);
        });

    source.attach(main_context);
}

void mir::GLibMainLoop::enqueue(void const*, ServerAction const& action)
{
    auto const source = make_idle_source(G_PRIORITY_DEFAULT, action);
    source.attach(main_context);
}
