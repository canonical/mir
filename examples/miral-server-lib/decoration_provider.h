/*
 * Copyright © 2016-2017 Canonical Ltd.
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

#include <mir/client/connection.h>
#include <mir/client/surface.h>
#include <mir/client/window.h>

#include <mir/geometry/rectangle.h>
#include <mir_toolkit/client_types.h>

#include <atomic>
#include <map>
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

    void operator()(mir::client::Connection connection);
    void operator()(std::weak_ptr<mir::scene::Session> const& session);

    auto session() const -> std::shared_ptr<mir::scene::Session>;

    void create_titlebar_for(miral::Window const& window);
    void place_new_decoration(miral::WindowSpecification& window_spec);
    void paint_titlebar_for(miral::WindowInfo const& window, int intensity);
    void destroy_titlebar_for(miral::Window const& window);
    void resize_titlebar_for(miral::WindowInfo const& window_info, mir::geometry::Size const& size);
    void advise_new_titlebar(miral::WindowInfo const& window_info);
    void advise_state_change(miral::WindowInfo const& window_info, MirWindowState state);

    void stop();

    bool is_decoration(miral::Window const& window) const;
    bool is_titlebar(miral::WindowInfo const& window_info) const;

private:
    struct Data
    {
        MirConnection* connection{nullptr};
        mir::client::Surface surface;
        MirBufferStream* stream{nullptr};
        std::atomic<MirWindow*> titlebar{nullptr};
        std::atomic<int> intensity{0xff};
        std::function<void(MirWindow* surface)> on_create{[](MirWindow*){}};
        miral::Window window;

        ~Data();
    };

    using SurfaceMap = std::map<std::weak_ptr<mir::scene::Surface>, Data, std::owner_less<std::weak_ptr<mir::scene::Surface>>>;
    using TitleMap = std::map<std::string, std::weak_ptr<mir::scene::Surface>>;

    miral::WindowManagerTools tools;
    std::mutex mutable mutex;
    mir::client::Connection connection;
    struct Wallpaper { mir::client::Surface surface; mir::client::Window window; MirBufferStream* stream; };
    std::vector<Wallpaper> wallpaper;
    std::weak_ptr<mir::scene::Session> weak_session;

    SurfaceMap window_to_titlebar;
    TitleMap windows_awaiting_titlebar;

    static void insert(MirWindow* surface, Data* data);
    Data* find_titlebar_data(miral::Window const& window);
    miral::Window find_titlebar_window(miral::Window const& window) const;
    void repaint_titlebar_for(miral::WindowInfo const& window_info);
    static void handle_event(MirWindow* window, MirEvent const* ev, void* context_);
    static void handle_event_for_background(MirWindow* window, MirEvent const* event, void* context_);
    void handle_event_for_background(MirWindow* window, MirEvent const* ev);
};


#endif //MIRAL_SHELL_DECORATION_PROVIDER_H
