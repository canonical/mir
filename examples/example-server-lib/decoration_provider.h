/*
 * Copyright © 2016-2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_SHELL_DECORATION_PROVIDER_H
#define MIRAL_SHELL_DECORATION_PROVIDER_H


#include <miral/window_manager_tools.h>

//#include <mir/client/connection.h>
//#include <mir/client/surface.h>
//#include <mir/client/window.h>
//
//#include <mir/geometry/rectangle.h>
//#include <mir_toolkit/client_types.h>
//
//#include <atomic>
//#include <map>
#include <mutex>
#include <condition_variable>
#include <queue>

class Worker
{
public:
    ~Worker();

    void start_work();
    void enqueue_work(std::function<void()> const& functor);
    void stop_work();

private:
    using WorkQueue = std::queue<std::function<void()>>;

    std::mutex mutable work_mutex;
    std::condition_variable work_cv;
    WorkQueue work_queue;
    bool work_done = false;

    void do_work();
};

class DecorationProvider : Worker
{
public:
    DecorationProvider(miral::WindowManagerTools const& tools);
    ~DecorationProvider();

    void operator()(struct wl_display* display);
    void operator()(std::weak_ptr<mir::scene::Session> const& session);

    auto session() const -> std::shared_ptr<mir::scene::Session>;

    void stop();

    bool is_decoration(miral::Window const& window) const;

private:
    struct Self;
    std::shared_ptr<Self> const self;

//    miral::WindowManagerTools tools;
//    std::mutex mutable mutex;
//    mir::client::Connection connection;
//    struct Wallpaper { mir::client::Surface surface; mir::client::Window window; MirBufferStream* stream; };
//    std::vector<Wallpaper> wallpaper;
//    std::weak_ptr<mir::scene::Session> weak_session;
//
//    static void handle_event_for_background(MirWindow* window, MirEvent const* event, void* context_);
//    void handle_event_for_background(MirWindow* window, MirEvent const* ev);
};


#endif //MIRAL_SHELL_DECORATION_PROVIDER_H
