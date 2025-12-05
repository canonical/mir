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

#include "foreign_toplevel_list_v1.h"

#include "wayland_utils.h"
#include "desktop_file_manager.h"
#include "mir/wayland/weak.h"
#include "mir/frontend/surface_stack.h"
#include "mir/shell/surface_specification.h"
#include "mir/scene/null_observer.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/scene/surface.h"
#include "mir/scene/session.h"
#include "mir/log.h"
#include "mir/executor.h"

#include <format>
#include <map>
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mw = mir::wayland;

namespace mir
{
namespace frontend
{
class ExtForeignToplevelListV1;
class ExtForeignToplevelHandleV1;

namespace
{
class ForeignSurfaceObserver;

// A helper class to keep track of toplevel IDs
class ForeignToplevelIdentifierMap
{
public:
    auto toplevel_id(std::shared_ptr<scene::Surface> const& surface) -> std::string;
    void forget_toplevel(std::shared_ptr<scene::Surface> const& surface);
    void forget_stale_toplevels();

private:
    std::map<
        std::weak_ptr<scene::Surface>,
        std::string,
        std::owner_less<std::weak_ptr<scene::Surface>>> toplevel_ids;
    uint64_t next_id = 0;
};

class ForeignSceneObserver
    : public ms::NullObserver,
      public std::enable_shared_from_this<ForeignSceneObserver>
{
public:
    ForeignSceneObserver(
        std::shared_ptr<Executor> const& wayland_executor,
        ExtForeignToplevelListV1* manager,
        std::shared_ptr<DesktopFileManager> const& desktop_file_manager,
        std::shared_ptr<ForeignToplevelIdentifierMap> const& id_map);
    ~ForeignSceneObserver();

private:
    /// Shell observer
    ///@{
    void surface_added(std::shared_ptr<scene::Surface> const& surface) override;
    void surface_removed(std::shared_ptr<scene::Surface> const& surface) override;
    void surface_exists(std::shared_ptr<scene::Surface> const& surface) override;
    void end_observation() override;
    ///@}

    void create_surface_observer(std::shared_ptr<scene::Surface> const& surface);
    void destroy_surface_observer(std::shared_ptr<scene::Surface> const& surface);
    void clear_surface_observers();

    std::shared_ptr<Executor> const wayland_executor;
    wayland::Weak<ExtForeignToplevelListV1> const manager;
    std::map<
        std::weak_ptr<scene::Surface>,
        std::shared_ptr<ForeignSurfaceObserver>,
        std::owner_less<std::weak_ptr<scene::Surface>>> surface_observers;

    std::shared_ptr<DesktopFileManager> const desktop_file_manager;
    std::shared_ptr<ForeignToplevelIdentifierMap> const id_map;
};

/// Bound by a client in order to get notified of toplevels from other clients via ForeignToplevelHandleV1
class ForeignSurfaceObserver
    : public scene::NullSurfaceObserver
{
public:
    ForeignSurfaceObserver(
        wayland::Weak<ExtForeignToplevelListV1> manager,
        std::shared_ptr<scene::Surface> const& surface,
        std::shared_ptr<DesktopFileManager> const& desktop_file_manager,
        std::shared_ptr<ForeignToplevelIdentifierMap> const& id_map);
    ~ForeignSurfaceObserver();

    void cease_and_desist(); ///< Must NOT be called under lock

private:
    /// if we don't have a live handle, action is not called and no error is raised
    void with_toplevel_handle(std::function<void(ExtForeignToplevelHandleV1&)> const& action);
    void create_or_close_toplevel_handle_as_needed();

    /// Surface observer
    ///@{
    void attrib_changed(scene::Surface const*, MirWindowAttrib attrib, int) override;
    void renamed(scene::Surface const*, std::string const& name) override;
    void application_id_set_to(scene::Surface const*, std::string const& application_id) override;
    ///@}

    wayland::Weak<ExtForeignToplevelListV1> const manager;

    std::weak_ptr<scene::Surface> weak_surface;
    /// If nullptr, the surface is not supposed to have a handle (such as when it does not have a toplevel type)
    /// If it points to an empty Weak, the handle is being created or was destroyed by the client
    std::shared_ptr<wayland::Weak<ExtForeignToplevelHandleV1>> handle;

    std::shared_ptr<DesktopFileManager> const desktop_file_manager;
    std::shared_ptr<ForeignToplevelIdentifierMap> const id_map;
};
}

/// Informs a client about toplevels from itself and other clients
/// The Wayland objects it creates for each toplevel can be used to aquire information and control that toplevel
/// Useful for task bars and app switchers
class ExtForeignToplevelListV1Global
    : public wayland::ExtForeignToplevelListV1::Global
{
public:
    ExtForeignToplevelListV1Global(
        wl_display* display,
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<SurfaceStack> const& surface_stack,
        std::shared_ptr<DesktopFileManager> const& desktop_file_manager);

    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<SurfaceStack> const surface_stack;
    std::shared_ptr<DesktopFileManager> const desktop_file_manager;
    std::shared_ptr<ForeignToplevelIdentifierMap> const id_map;

private:
    void bind(wl_resource* new_resource) override;
};

class ExtForeignToplevelListV1
    : public wayland::ExtForeignToplevelListV1
{
public:
    ExtForeignToplevelListV1(wl_resource* new_resource, ExtForeignToplevelListV1Global& global);
    ~ExtForeignToplevelListV1();

private:
    /// Wayland requests
    ///@{
    void stop() override;
    ///@}

    std::shared_ptr<SurfaceStack> const surface_stack;
    std::shared_ptr<ForeignSceneObserver> observer;
};

}
}

auto mf::create_ext_foreign_toplevel_list_v1(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<SurfaceStack> const& surface_stack,
    std::shared_ptr<DesktopFileManager> const& desktop_file_manager)
-> std::shared_ptr<mw::ExtForeignToplevelListV1::Global>
{
    return std::make_shared<ExtForeignToplevelListV1Global>(display, wayland_executor, surface_stack, desktop_file_manager);
}

// ExtForeignToplevelListV1Global

mf::ExtForeignToplevelListV1Global::ExtForeignToplevelListV1Global(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<SurfaceStack> const& surface_stack,
    std::shared_ptr<DesktopFileManager> const& desktop_file_manager)
    : Global{display, Version<1>()},
      wayland_executor{wayland_executor},
      surface_stack{surface_stack},
      desktop_file_manager{desktop_file_manager},
      id_map{std::make_shared<ForeignToplevelIdentifierMap>()}
{
}

void mf::ExtForeignToplevelListV1Global::bind(wl_resource* new_resource)
{
    new ExtForeignToplevelListV1{new_resource, *this};
}


// ForeignToplevelIdentifierMap

std::string mf::ForeignToplevelIdentifierMap::toplevel_id(std::shared_ptr<scene::Surface> const& surface)
{
    std::string& identifier = toplevel_ids[surface];
    if (identifier.empty())
    {
        identifier = std::format("toplevel:{:x}", next_id++);
    }
    return identifier;
}

void mf::ForeignToplevelIdentifierMap::forget_toplevel(std::shared_ptr<scene::Surface> const& surface)
{
    toplevel_ids.erase(surface);
}

void mf::ForeignToplevelIdentifierMap::forget_stale_toplevels()
{
    std::erase_if(toplevel_ids, [](auto const& item)
        {
            return item.first.expired();
        });
}

// ForeignSceneObserver

mf::ForeignSceneObserver::ForeignSceneObserver(
    std::shared_ptr<Executor> const& wayland_executor,
    ExtForeignToplevelListV1* manager,
    std::shared_ptr<DesktopFileManager> const& desktop_file_manager,
    std::shared_ptr<ForeignToplevelIdentifierMap> const& id_map)
    : wayland_executor{wayland_executor},
      manager{manager},
      desktop_file_manager{desktop_file_manager},
      id_map{id_map}
{
    // Forget toplevel IDs for windows closed while no observer was
    // running.
    id_map->forget_stale_toplevels();
}

mf::ForeignSceneObserver::~ForeignSceneObserver()
{
    clear_surface_observers();
}

void mf::ForeignSceneObserver::surface_added(std::shared_ptr<scene::Surface> const& surface)
{
    // Defer processing to Wayland thread
    wayland_executor->spawn(
        [weak_observer = weak_from_this(), surface]()
            {
                if (auto const observer = weak_observer.lock())
                {
                    observer->create_surface_observer(surface);
                }
            });
}

void mf::ForeignSceneObserver::surface_removed(std::shared_ptr<scene::Surface> const& surface)
{
    // Defer processing to Wayland thread
    wayland_executor->spawn(
        [weak_observer = weak_from_this(), surface]()
            {
                if (auto const observer = weak_observer.lock())
                {
                    observer->destroy_surface_observer(surface);
                }
            });
}

void mf::ForeignSceneObserver::surface_exists(std::shared_ptr<scene::Surface> const& surface)
{
    // Defer processing to Wayland thread
    wayland_executor->spawn(
        [weak_observer = weak_from_this(), surface]()
            {
                if (auto const observer = weak_observer.lock())
                {
                    observer->create_surface_observer(surface);
                }
            });
}

void mf::ForeignSceneObserver::end_observation()
{
    // Defer processing to Wayland thread
    wayland_executor->spawn(
        [weak_observer = weak_from_this()]()
            {
                if (auto const observer = weak_observer.lock())
                {
                    observer->clear_surface_observers();
                }
            });
}

void mf::ForeignSceneObserver::create_surface_observer(std::shared_ptr<scene::Surface> const& surface)
{
    auto observer = std::make_shared<ForeignSurfaceObserver>(manager, surface, desktop_file_manager, id_map);
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

void mf::ForeignSceneObserver::destroy_surface_observer(std::shared_ptr<scene::Surface> const& surface)
{
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
    id_map->forget_toplevel(surface);
}

void mf::ForeignSceneObserver::clear_surface_observers()
{
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
    mw::Weak<ExtForeignToplevelListV1> manager,
    std::shared_ptr<scene::Surface> const& surface,
    std::shared_ptr<DesktopFileManager> const& desktop_file_manager,
    std::shared_ptr<ForeignToplevelIdentifierMap> const& id_map)
    : manager{manager},
      weak_surface{surface},
      desktop_file_manager{desktop_file_manager},
      id_map{id_map}
{
    create_or_close_toplevel_handle_as_needed();
}

mf::ForeignSurfaceObserver::~ForeignSurfaceObserver()
{
    cease_and_desist();
}

void mf::ForeignSurfaceObserver::cease_and_desist()
{
    weak_surface.reset();
    create_or_close_toplevel_handle_as_needed();
}

void mf::ForeignSurfaceObserver::with_toplevel_handle(
    std::function<void(ExtForeignToplevelHandleV1&)> const& action)
{
    if (handle && *handle)
    {
        action(handle->value());
    }
}

void mf::ForeignSurfaceObserver::create_or_close_toplevel_handle_as_needed()
{
    bool should_have_handle = true;

    auto const surface = weak_surface.lock();
    if (surface)
    {
        switch(surface->state())
        {
        case mir_window_state_attached:
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
            // If the manager has been destroyed we can't create a toplevel handle
            if (!manager)
                return;

            handle = std::make_shared<mw::Weak<ExtForeignToplevelHandleV1>>();

            std::string toplevel_id = id_map->toplevel_id(surface);
            std::string name = surface->name();
            std::string app_id = desktop_file_manager->resolve_app_id(surface.get());

            // Remember Wayland objects manage their own lifetime
            auto const handle_ptr = new ExtForeignToplevelHandleV1{manager.value(), surface};
            *handle = mw::make_weak(handle_ptr);

            handle->value().send_identifier_event(toplevel_id);
            if (!name.empty())
                handle->value().send_title_event(name);
            if (!app_id.empty())
                handle->value().send_app_id_event(app_id);
            handle->value().send_done_event();
        }
        else
        {
            with_toplevel_handle([](ExtForeignToplevelHandleV1& handle)
                {
                    handle.should_close();
                });
            handle.reset();
        }
    }
}

void mf::ForeignSurfaceObserver::attrib_changed(const scene::Surface*, MirWindowAttrib attrib, int)
{
    auto surface = weak_surface.lock();
    if (!surface)
    {
        return;
    }

    switch (attrib)
    {
    case mir_window_attrib_state:
    case mir_window_attrib_type:
        create_or_close_toplevel_handle_as_needed();

    default:
        break;
    }
}

void mf::ForeignSurfaceObserver::renamed(ms::Surface const*, std::string const& name)
{
    with_toplevel_handle([name](ExtForeignToplevelHandleV1& handle)
        {
            handle.send_title_event(name);
            handle.send_done_event();
        });
}

void mf::ForeignSurfaceObserver::application_id_set_to(
    scene::Surface const* surface,
    std::string const& application_id)
{
    std::string id = application_id;
    with_toplevel_handle([&](ExtForeignToplevelHandleV1& handle)
        {
            auto app_id = desktop_file_manager->resolve_app_id(surface);
            if (!app_id.empty())
            {
                handle.send_app_id_event(app_id);
                handle.send_done_event();
            }
        });
}

// ExtForeignToplevelListV1

mf::ExtForeignToplevelListV1::ExtForeignToplevelListV1(
    wl_resource* new_resource,
    ExtForeignToplevelListV1Global& global)
    : mw::ExtForeignToplevelListV1{new_resource, Version<1>()},
      surface_stack{global.surface_stack},
      observer{std::make_shared<ForeignSceneObserver>(global.wayland_executor, this, global.desktop_file_manager, global.id_map)}
{
    surface_stack->add_observer(observer);
}

mf::ExtForeignToplevelListV1::~ExtForeignToplevelListV1()
{
    if (observer)
    {
        surface_stack->remove_observer(observer);
    }
}

void mf::ExtForeignToplevelListV1::stop()
{
    if (observer)
    {
        send_finished_event();
        surface_stack->remove_observer(observer);
        observer.reset();
    }
}

// ExtForeignToplevelHandleV1

mf::ExtForeignToplevelHandleV1::ExtForeignToplevelHandleV1(
    ExtForeignToplevelListV1 const& manager,
    std::shared_ptr<ms::Surface> const& surface)
    : mw::ExtForeignToplevelHandleV1{manager},
      weak_surface{surface}
{
    manager.send_toplevel_event(resource);
}

auto mf::ExtForeignToplevelHandleV1::from(
    wl_resource* resource) -> ExtForeignToplevelHandleV1*
{
    return dynamic_cast<ExtForeignToplevelHandleV1*>(wayland::ExtForeignToplevelHandleV1::from(resource));
}

auto mf::ExtForeignToplevelHandleV1::from_or_throw(
    wl_resource* resource) -> ExtForeignToplevelHandleV1&
{
    auto const handle = ExtForeignToplevelHandleV1::from(resource);
    if (!handle)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "ext_foreign_toplevel_handle_v1@" +
            std::to_string(wl_resource_get_id(resource)) +
            " is not a mir::frontend::ExtForeignToplevelHandleV1"));
    }
    return *handle;
}

auto mf::ExtForeignToplevelHandleV1::surface() const -> std::shared_ptr<ms::Surface>
{
    return weak_surface.lock();
}

void mf::ExtForeignToplevelHandleV1::should_close()
{
    send_closed_event();
    weak_surface.reset();
}
