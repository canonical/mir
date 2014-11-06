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
#include "mir/glib_main_loop_sources.h"

#include <stdexcept>

#include <boost/throw_exception.hpp>

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
    auto const gsource = detail::make_idle_gsource(G_PRIORITY_HIGH,
        [this]
        {
            running = false;
            g_main_context_wakeup(main_context);
        });

    gsource.attach(main_context);
}

void mir::GLibMainLoop::register_signal_handler(
    std::initializer_list<int> sigs,
    std::function<void(int)> const& handler)
{
    for (auto sig : sigs)
    {
        auto const gsource = detail::make_signal_gsource(sig, handler);
        gsource.attach(main_context);
    }
}

void mir::GLibMainLoop::enqueue(void const*, ServerAction const& action)
{
    auto const gsource = detail::make_idle_gsource(G_PRIORITY_DEFAULT, action);
    gsource.attach(main_context);
}
