/*
 * Copyright © Canonical Ltd.
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
 */

#include "foreign_toplevel_manager_v1.h"

#include "wayland_utils.h"
#include "desktop_file_manager.h"
#include "mir/wayland/weak.h"
#include "mir/frontend/surface_stack.h"
#include "mir/shell/shell.h"
#include "mir/shell/surface_specification.h"
#include "mir/scene/null_observer.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/scene/surface.h"
#include "mir/scene/session.h"
#include "mir/log.h"
#include "mir/executor.h"
#include "mir/main_loop.h"

#include <algorithm>
#include <mutex>
#include <map>
#include <boost/throw_exception.hpp>
#include <fstream>
#include <gio/gdesktopappinfo.h>
#include <filesystem>
#include <sys/inotify.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mw = mir::wayland;

namespace mir
{
namespace frontend
{
class ForeignSceneObserver;
class ForeignSurfaceObserver;
class ForeignToplevelManagerV1;
class ForeignToplevelHandleV1;

class GDesktopFileCache : public DesktopFileCache
{
public:
    GDesktopFileCache(std::shared_ptr<MainLoop> const& main_loop);
    ~GDesktopFileCache() override = default;
    std::shared_ptr<DesktopFile> lookup_by_app_id(std::string const&) const override;
    std::shared_ptr<DesktopFile> lookup_by_wm_class(std::string const&) const override;
    std::vector<std::shared_ptr<DesktopFile>> const& get_desktop_files() const override;
    void refresh_app_cache();

private:
    std::shared_ptr<MainLoop> const main_loop;
    mir::Fd inotify_fd;
    std::vector<int> config_path_wd_list;
    std::vector<std::shared_ptr<DesktopFile>> files;
    std::map<std::string, std::shared_ptr<DesktopFile>> id_to_app;
    std::map<std::string, std::string> wm_class_to_app_info_id;
};

/// Informs a client about toplevels from itself and other clients
/// The Wayland objects it creates for each toplevel can be used to aquire information and control that toplevel
/// Useful for task bars and app switchers
class ForeignToplevelManagerV1Global
    : public wayland::ForeignToplevelManagerV1::Global
{
public:
    ForeignToplevelManagerV1Global(
        wl_display* display,
        std::shared_ptr<shell::Shell> const& shell,
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<SurfaceStack> const& surface_stack,
        std::shared_ptr<MainLoop> const& main_loop);

    std::shared_ptr<shell::Shell> const shell;
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<SurfaceStack> const surface_stack;
    std::shared_ptr<DesktopFileManager> const desktop_file_manager;

private:
    void bind(wl_resource* new_resource) override;
};

class ForeignSceneObserver
    : public ms::NullObserver
{
public:
    ForeignSceneObserver(std::shared_ptr<Executor> const& wayland_executor, ForeignToplevelManagerV1* manager);
    ~ForeignSceneObserver();

private:
    /// Shell observer
    ///@{
    void surface_added(std::shared_ptr<scene::Surface> const& surface) override;
    void surface_removed(std::shared_ptr<scene::Surface> const& surface) override;
    void surface_exists(std::shared_ptr<scene::Surface> const& surface) override;
    void end_observation() override;
    ///@}

    void create_surface_observer(std::shared_ptr<scene::Surface> const& surface); ///< Should NOT be called under lock
    void clear_surface_observers(); ///< Should NOT be called under lock

    std::shared_ptr<Executor> const wayland_executor;
    wayland::Weak<ForeignToplevelManagerV1> const manager; ///< Can only be safely accessed on the Wayland thread
    std::mutex mutex;
    std::map<
        std::weak_ptr<scene::Surface>,
        std::shared_ptr<ForeignSurfaceObserver>,
        std::owner_less<std::weak_ptr<scene::Surface>>> surface_observers;
};

/// Bound by a client in order to get notified of toplevels from other clients via ForeignToplevelHandleV1
class ForeignSurfaceObserver
    : public scene::NullSurfaceObserver
{
public:
    ForeignSurfaceObserver(
        wayland::Weak<ForeignToplevelManagerV1> manager,
        std::shared_ptr<scene::Surface> const& surface);
    ~ForeignSurfaceObserver();

    void cease_and_desist(); ///< Must NOT be called under lock

private:
    /// if we don't have a live handle, action is not called and no error is raised
    void with_toplevel_handle(std::lock_guard<std::mutex>&, std::function<void(ForeignToplevelHandleV1&)>&& action);
    void create_or_close_toplevel_handle_as_needed(std::lock_guard<std::mutex>& lock);

    /// Surface observer
    ///@{
    void attrib_changed(scene::Surface const*, MirWindowAttrib attrib, int) override;
    void renamed(scene::Surface const*, std::string const& name) override;
    void application_id_set_to(scene::Surface const*, std::string const& application_id) override;
    ///@}

    wayland::Weak<ForeignToplevelManagerV1> const manager;

    std::mutex mutex;
    std::weak_ptr<scene::Surface> weak_surface;
    /// If nullptr, the surface is not supposed to have a handle (such as when it does not have a toplevel type)
    /// If it points to an empty Weak, the handle is being created or was destroyed by the client
    std::shared_ptr<wayland::Weak<ForeignToplevelHandleV1>> handle;
};

class ForeignToplevelManagerV1
    : public wayland::ForeignToplevelManagerV1
{
public:
    ForeignToplevelManagerV1(wl_resource* new_resource, ForeignToplevelManagerV1Global& global);
    ~ForeignToplevelManagerV1();

    std::shared_ptr<shell::Shell> const shell;
    std::shared_ptr<DesktopFileManager> const desktop_file_manager;

private:
    /// Wayland requests
    ///@{
    void stop() override;
    ///@}

    std::shared_ptr<SurfaceStack> const surface_stack;
    std::shared_ptr<ForeignSceneObserver> const observer;
};

/// Used by a client to aquire information about or control a specific toplevel
class ForeignToplevelHandleV1
    : public wayland::ForeignToplevelHandleV1
{
public:
    ForeignToplevelHandleV1(ForeignToplevelManagerV1 const& manager, std::shared_ptr<scene::Surface> const& surface);

    /// Sends the required .state event
    void send_state(MirWindowFocusState focused, ms::SurfaceStateTracker state);

    /// Sends the .closed event and makes this surface inert
    void should_close();

private:
    /// Modifies the surface state if possible, silently fails if not
    void attempt_change_surface_state(MirWindowState state, bool has_state);

    /// Wayland requests
    ///@{
    void set_maximized() override;
    void unset_maximized() override;
    void set_minimized() override;
    void unset_minimized() override;
    void activate(struct wl_resource* seat) override;
    void close() override;
    void set_rectangle(struct wl_resource* surface, int32_t x, int32_t y, int32_t width, int32_t height) override;
    void set_fullscreen(std::optional<struct wl_resource*> const& output) override;
    void unset_fullscreen() override;
    ///@}

    std::weak_ptr<shell::Shell> const weak_shell;
    std::weak_ptr<scene::Surface> weak_surface;
};

}
}

auto mf::create_foreign_toplevel_manager_v1(
    wl_display* display,
    std::shared_ptr<shell::Shell> const& shell,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<SurfaceStack> const& surface_stack,
    std::shared_ptr<MainLoop> const& main_loop)
-> std::shared_ptr<mw::ForeignToplevelManagerV1::Global>
{
    return std::make_shared<ForeignToplevelManagerV1Global>(display, shell, wayland_executor, surface_stack, main_loop);
}

// ForeignToplevelManagerV1Global

mf::ForeignToplevelManagerV1Global::ForeignToplevelManagerV1Global(
    wl_display* display,
    std::shared_ptr<shell::Shell> const& shell,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<SurfaceStack> const& surface_stack,
    std::shared_ptr<MainLoop> const& main_loop)
    : Global{display, Version<2>()},
      shell{shell},
      wayland_executor{wayland_executor},
      surface_stack{surface_stack},
      desktop_file_manager{std::make_shared<DesktopFileManager>(std::make_shared<GDesktopFileCache>(main_loop))}
{
}

void mf::ForeignToplevelManagerV1Global::bind(wl_resource* new_resource)
{
    new ForeignToplevelManagerV1{new_resource, *this};
}

// ForeignSceneObserver

mf::ForeignSceneObserver::ForeignSceneObserver(
    std::shared_ptr<Executor> const& wayland_executor,
    ForeignToplevelManagerV1* manager)
    : wayland_executor{wayland_executor},
      manager{manager}
{
}

mf::ForeignSceneObserver::~ForeignSceneObserver()
{
    clear_surface_observers();
}

void mf::ForeignSceneObserver::surface_added(std::shared_ptr<scene::Surface> const& surface)
{
    create_surface_observer(surface);
}

void mf::ForeignSceneObserver::surface_removed(std::shared_ptr<scene::Surface> const& surface)
{
    std::lock_guard lock{mutex};
    auto const iter = surface_observers.find(surface);
    if (iter == surface_observers.end())
    {
        log_error(
            "Can not remove ForeignSurfaceObserver: surface %p not in observers map",
            static_cast<void*>(surface.get()));
    }
    else
    {
        iter->second->cease_and_desist();
        surface_observers.erase(iter);
    }
}

void mf::ForeignSceneObserver::surface_exists(std::shared_ptr<scene::Surface> const& surface)
{
    create_surface_observer(surface);
}

void mf::ForeignSceneObserver::end_observation()
{
    clear_surface_observers();
}

void mf::ForeignSceneObserver::create_surface_observer(std::shared_ptr<scene::Surface> const& surface)
{
    std::lock_guard lock{mutex};
    auto observer = std::make_shared<ForeignSurfaceObserver>(manager, surface);
    surface->register_interest(observer, *wayland_executor);
    auto insert_result = surface_observers.insert(std::make_pair(surface, observer));
    if (!insert_result.second)
    {
        log_error(
            "Can not add ForeignSurfaceObserver: surface %p already in the observers map",
            static_cast<void*>(surface.get()));
        observer->cease_and_desist();
    }
}

void mf::ForeignSceneObserver::clear_surface_observers()
{
    std::lock_guard lock{mutex};
    for (auto const& pair : surface_observers)
    {
        pair.second->cease_and_desist();
        if (auto const surface = pair.first.lock())
        {
            surface->unregister_interest(*pair.second);
        }
    }
    surface_observers.clear();
}

// ForeignSurfaceObserver

mf::ForeignSurfaceObserver::ForeignSurfaceObserver(
    mw::Weak<ForeignToplevelManagerV1> manager,
    std::shared_ptr<scene::Surface> const& surface)
    : manager{manager},
      weak_surface{surface}
{
    std::lock_guard lock{mutex};
    create_or_close_toplevel_handle_as_needed(lock);
}

mf::ForeignSurfaceObserver::~ForeignSurfaceObserver()
{
    cease_and_desist();
}

void mf::ForeignSurfaceObserver::cease_and_desist()
{
    std::lock_guard lock{mutex};
    weak_surface.reset();
    create_or_close_toplevel_handle_as_needed(lock);
}

void mf::ForeignSurfaceObserver::with_toplevel_handle(
    std::lock_guard<std::mutex>&,
    std::function<void(ForeignToplevelHandleV1&)>&& action)
{
    if (handle && *handle)
    {
        action(handle->value());
    }
}

void mf::ForeignSurfaceObserver::create_or_close_toplevel_handle_as_needed(std::lock_guard<std::mutex>& lock)
{
    bool should_have_handle = true;

    auto const surface = weak_surface.lock();
    if (surface)
    {
        switch(surface->state())
        {
        case mir_window_state_attached:
        case mir_window_state_hidden:
            should_have_handle = false;
            break;

        default:
            break;
        }

        switch (surface->type())
        {
        case mir_window_type_normal:
        case mir_window_type_utility:
        case mir_window_type_freestyle:
            break;

        default:
            should_have_handle = false;
            break;
        }

        if (!surface->session().lock())
            should_have_handle = false;
    }
    else
    {
        should_have_handle = false;
    }

    bool const currently_have_handle{handle};
    if (should_have_handle != currently_have_handle)
    {
        if (should_have_handle)
        {
            handle = std::make_shared<mw::Weak<ForeignToplevelHandleV1>>();

            std::string name = surface->name();
            std::string app_id = manager.value().desktop_file_manager->resolve_app_id(surface.get());
            auto const focused = surface->focus_state();
            auto const state = surface->state_tracker();

            // If the manager has been destroyed we can't create a toplevel handle
            if (!manager)
                return;

            // Remember Wayland objects manage their own lifetime
            auto const handle_ptr = new ForeignToplevelHandleV1{manager.value(), surface};
            *handle = mw::make_weak(handle_ptr);

            if (!name.empty())
                handle->value().send_title_event(name);
            handle->value().send_app_id_event(app_id);
            handle->value().send_state(focused, state);
            handle->value().send_done_event();
        }
        else
        {
            with_toplevel_handle(lock, [](ForeignToplevelHandleV1& handle)
                {
                    handle.should_close();
                });
            handle = {};
        }
    }
}

void mf::ForeignSurfaceObserver::attrib_changed(const scene::Surface*, MirWindowAttrib attrib, int)
{
    std::lock_guard lock{mutex};

    auto surface = weak_surface.lock();
    if (!surface)
    {
        return;
    }

    bool const toplevel_handel_existed_before{handle};

    switch (attrib)
    {
    case mir_window_attrib_state:
    case mir_window_attrib_type:
        create_or_close_toplevel_handle_as_needed(lock);

    default:
        break;
    }

    if (!toplevel_handel_existed_before)
    {
        // If we just created the handle, no need to send updates
        return;
    }

    switch (attrib)
    {
    case mir_window_attrib_state:
    case mir_window_attrib_focus:
    {
        auto const focused = surface->focus_state();
        auto const state = surface->state_tracker();
        with_toplevel_handle(lock, [focused, state](ForeignToplevelHandleV1& handle)
            {
                handle.send_state(focused, state);
                handle.send_done_event();
            });
    }   break;

    default:
        break;
    }
}

void mf::ForeignSurfaceObserver::renamed(ms::Surface const*, std::string const& name)
{
    std::lock_guard lock{mutex};

    with_toplevel_handle(lock, [name](ForeignToplevelHandleV1& handle)
        {
            handle.send_title_event(name);
            handle.send_done_event();
        });
}

void mf::ForeignSurfaceObserver::application_id_set_to(
    scene::Surface const* surface,
    std::string const& application_id)
{
    std::lock_guard lock{mutex};

    std::string id = application_id;
    with_toplevel_handle(lock, [&](ForeignToplevelHandleV1& handle)
        {
            auto app_id = manager.value().desktop_file_manager->resolve_app_id(surface);
            handle.send_app_id_event(app_id);
            handle.send_done_event();
        });
}

// GDesktopFileCache
mf::GDesktopFileCache::GDesktopFileCache(const std::shared_ptr<MainLoop> &main_loop)
    : main_loop{main_loop},
      inotify_fd{inotify_init()}
{
    refresh_app_cache();

    if (inotify_fd < 0)
    {
        mir::log_error("Unable to initialize inotify. Automatic desktop app refresh will NOT happen.");
        return;
    }

    // Set up a watch on the data directories
    std::vector<std::string> data_directories;
    if (const char* xdg_data_dirs = std::getenv("XDG_DATA_DIRS"))
    {
        std::string directory;
        for (size_t i = 0; i < strlen(xdg_data_dirs); i++) {
            if (xdg_data_dirs[i] == ':') {
                data_directories.push_back(directory);
                directory = "";
                continue;
            }
            directory += xdg_data_dirs[i];
        }
    }
    else
    {
        data_directories = {"/usr/local/share", "/usr/share"};
        if (auto home = getenv("HOME"))
            data_directories.push_back(std::string(home) + "/.local/share");

        if (const char* xdg_data_home_dir = std::getenv("XDG_DATA_HOME"))
            data_directories.push_back(xdg_data_home_dir);
    }

    for (auto directory : data_directories)
    {
        auto application_directory = directory + "/applications";
        int config_path_wd = inotify_add_watch(
            inotify_fd,
            application_directory.c_str(),
            IN_CREATE | IN_DELETE | IN_CLOSE_WRITE | IN_MOVED_TO | IN_MOVED_FROM);

        if (config_path_wd < 0)
        {
            mir::log_error("Unable to watch directory %s", application_directory.c_str());
            continue;
        }

        config_path_wd_list.push_back(config_path_wd);
    }

    main_loop->register_fd_handler({inotify_fd}, this, [this](int)
    {
        union
        {
            inotify_event event;
            char buffer[sizeof(inotify_event) + NAME_MAX + 1];
        } inotify_buffer;
        if (read(inotify_fd, &inotify_buffer, sizeof(inotify_buffer)) < static_cast<ssize_t>(sizeof(inotify_event)))
            return;

        refresh_app_cache();
    });
}

void mf::GDesktopFileCache::refresh_app_cache()
{
    auto app_infos = g_app_info_get_all();
    id_to_app.clear();
    wm_class_to_app_info_id.clear();

    GList* info;
    for (info = app_infos; info != NULL; info = info->next)
    {
        GAppInfo *app_info = static_cast<GAppInfo*>(info->data);
        const bool no_display = g_desktop_app_info_get_nodisplay(G_DESKTOP_APP_INFO(app_info));
        if (no_display)
            continue;

        const char* id = g_app_info_get_id(app_info);
        const char* wm_class = g_desktop_app_info_get_startup_wm_class(G_DESKTOP_APP_INFO(app_info));
        const char* exec = g_app_info_get_executable(app_info);

        // Note: This can be null possibly. The only instance I've seen of this
        // is XWayland on Ubuntu 23, but I am sure others exist
        if (exec == NULL)
        {
            const char* name = g_app_info_get_name(app_info);
            if (name == NULL)
                mir::log_warning("Cannot find app info for unknown application");
            else
                mir::log_warning("Cannot find app info for app with name:" + std::string(name));
            continue;
        }

        std::shared_ptr<DesktopFile> file = std::make_shared<DesktopFile>(id, wm_class, exec);
        files.push_back(file);
        const char* app_id = g_app_info_get_id(app_info);
        id_to_app[app_id] = file;

        if (wm_class == NULL)
            continue;

        wm_class_to_app_info_id.insert(std::pair<std::string, std::string>(wm_class, app_id));
    }

    g_list_free_full(app_infos, g_object_unref);
}

std::shared_ptr<mf::DesktopFile> mf::GDesktopFileCache::lookup_by_app_id(std::string const& name) const
{
    auto app_pos = id_to_app.find(name);
    if (app_pos != id_to_app.end())
    {
        return app_pos->second;
    }

    return nullptr;
}

std::shared_ptr<mf::DesktopFile> mf::GDesktopFileCache::lookup_by_wm_class(std::string const& name) const
{
    auto wm_class_pos = wm_class_to_app_info_id.find(name);
    if (wm_class_pos != wm_class_to_app_info_id.end())
    {
        auto app_id = wm_class_pos->second;
        return lookup_by_app_id(app_id);
    }

    return nullptr;
}

const std::vector<std::shared_ptr<mf::DesktopFile>> &mf::GDesktopFileCache::get_desktop_files() const
{
    return files;
}

// ForeignToplevelManagerV1

mf::ForeignToplevelManagerV1::ForeignToplevelManagerV1(
    wl_resource* new_resource,
    ForeignToplevelManagerV1Global& global)
    : mw::ForeignToplevelManagerV1{new_resource, Version<2>()},
      shell{global.shell},
      desktop_file_manager{global.desktop_file_manager},
      surface_stack{global.surface_stack},
      observer{std::make_shared<ForeignSceneObserver>(global.wayland_executor, this)}
{
    surface_stack->add_observer(observer);
}

mf::ForeignToplevelManagerV1::~ForeignToplevelManagerV1()
{
    surface_stack->remove_observer(observer);
}

void mf::ForeignToplevelManagerV1::stop()
{
    send_finished_event();
    destroy_and_delete();
}

// ForeignToplevelHandleV1

mf::ForeignToplevelHandleV1::ForeignToplevelHandleV1(
    ForeignToplevelManagerV1 const& manager,
    std::shared_ptr<ms::Surface> const& surface)
    : mw::ForeignToplevelHandleV1{manager},
      weak_shell{manager.shell},
      weak_surface{surface}
{
    manager.send_toplevel_event(resource);
}

void mf::ForeignToplevelHandleV1::send_state(MirWindowFocusState focused, ms::SurfaceStateTracker state)
{
    wl_array states;
    wl_array_init(&states);

    if (focused == mir_window_focus_state_focused)
    {
        if (uint32_t* state = static_cast<uint32_t*>(wl_array_add(&states, sizeof(uint32_t))))
            *state = State::activated;
    }

    if (state.has(mir_window_state_horizmaximized) || state.has(mir_window_state_vertmaximized))
    {
        if (uint32_t *state = static_cast<uint32_t*>(wl_array_add(&states, sizeof(uint32_t))))
            *state = State::maximized;
    }

    if (state.has(mir_window_state_fullscreen))
    {
        if (uint32_t *state = static_cast<uint32_t*>(wl_array_add(&states, sizeof(uint32_t))))
            *state = State::fullscreen;
    }

    if (state.has(mir_window_state_minimized))
    {
        if (uint32_t *state = static_cast<uint32_t*>(wl_array_add(&states, sizeof(uint32_t))))
            *state = State::minimized;
    }

    send_state_event(&states);
    wl_array_release(&states);
}

void mf::ForeignToplevelHandleV1::should_close()
{
    send_closed_event();
    weak_surface.reset();
}

void mf::ForeignToplevelHandleV1::attempt_change_surface_state(MirWindowState state, bool has_state)
{
    auto const shell = weak_shell.lock();
    auto const surface = weak_surface.lock();
    auto const session = surface ? surface->session().lock() : nullptr;
    if (shell && session && surface)
    {
        msh::SurfaceSpecification spec;
        auto const tracker = surface->state_tracker();
        if (has_state)
        {
            spec.state = tracker.with_active_state(state).active_state();
        }
        else
        {
            spec.state = tracker.without(state).active_state();
        }
        if (spec.state.value() != tracker.active_state())
        {
            shell->modify_surface(session, surface, spec);
        }
    }
}

void mf::ForeignToplevelHandleV1::set_maximized()
{
    attempt_change_surface_state(mir_window_state_maximized, true);
}

void mf::ForeignToplevelHandleV1::unset_maximized()
{
    attempt_change_surface_state(mir_window_state_maximized, false);
}

void mf::ForeignToplevelHandleV1::set_minimized()
{
    attempt_change_surface_state(mir_window_state_minimized, true);
}

void mf::ForeignToplevelHandleV1::unset_minimized()
{
    attempt_change_surface_state(mir_window_state_minimized, false);
    activate(nullptr);
}

void mf::ForeignToplevelHandleV1::activate(struct wl_resource* /*seat*/)
{
    auto timestamp = std::numeric_limits<uint64_t>::max(); // TODO: make this correct
    auto const shell = weak_shell.lock();
    auto const surface = weak_surface.lock();
    auto const session = surface ? surface->session().lock() : nullptr;
    if (shell && session && surface)
    {
        shell->raise_surface(session, surface, timestamp);
    }
}

void mf::ForeignToplevelHandleV1::close()
{
    auto const shell = weak_shell.lock();
    auto const surface = weak_surface.lock();
    auto const session = surface ? surface->session().lock() : nullptr;
    if (shell && session && surface)
    {
        surface->request_client_surface_close();
    }
}

void mf::ForeignToplevelHandleV1::set_rectangle(
    struct wl_resource* /*surface*/,
    int32_t /*x*/,
    int32_t /*y*/,
    int32_t /*width*/,
    int32_t /*height*/)
{
    // This would be used for the destination of a window minimization animation
    // Nothing must be done with this info. It is not a protocol violation to ignore it
}

void mf::ForeignToplevelHandleV1::set_fullscreen(std::optional<struct wl_resource*> const& /*output*/)
{
    attempt_change_surface_state(mir_window_state_fullscreen, true);
    // TODO: respect output
}

void mf::ForeignToplevelHandleV1::unset_fullscreen()
{
    attempt_change_surface_state(mir_window_state_fullscreen, false);
}
