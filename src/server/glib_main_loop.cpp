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

#include <stdexcept>
#include <algorithm>

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
    : running{false},
      fd_sources{main_context}
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
    detail::add_idle_gsource(main_context, G_PRIORITY_HIGH,
        [this]
        {
            running = false;
            g_main_context_wakeup(main_context);
        });
}

void mir::GLibMainLoop::register_signal_handler(
    std::initializer_list<int> sigs,
    std::function<void(int)> const& handler)
{
    for (auto sig : sigs)
        detail::add_signal_gsource(main_context, sig, handler);
}

void mir::GLibMainLoop::register_fd_handler(
    std::initializer_list<int> fds,
    void const* owner,
    std::function<void(int)> const& handler)
{
    for (auto fd : fds)
        fd_sources.add(fd, owner, handler);
}

void mir::GLibMainLoop::unregister_fd_handler(
    void const* owner)
{
    fd_sources.remove_all_owned_by(owner);
}

void mir::GLibMainLoop::enqueue(void const* owner, ServerAction const& action)
{
    detail::add_server_action_gsource(main_context, owner, action,
        [this] (void const* owner)
        {
            return should_process_actions_for(owner);
        });
}

void mir::GLibMainLoop::pause_processing_for(void const* owner)
{
    std::lock_guard<std::mutex> lock{do_not_process_mutex};

    auto const iter = std::find(do_not_process.begin(), do_not_process.end(), owner);
    if (iter == do_not_process.end())
        do_not_process.push_back(owner);
}

void mir::GLibMainLoop::resume_processing_for(void const* owner)
{
    std::lock_guard<std::mutex> lock{do_not_process_mutex};

    auto const new_end = std::remove(do_not_process.begin(), do_not_process.end(), owner);
    do_not_process.erase(new_end, do_not_process.end());

    // Wake up the context to reprocess all sources
    g_main_context_wakeup(main_context);
}

bool mir::GLibMainLoop::should_process_actions_for(void const* owner)
{
    std::lock_guard<std::mutex> lock{do_not_process_mutex};

    auto const iter = std::find(do_not_process.begin(), do_not_process.end(), owner);
    return iter == do_not_process.end();
}
