/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "xdg_foreign_unstable_v2.h"
#include "wl_surface.h"

#include <mir/executor.h>
#include <mir/shell/surface_specification.h>
#include <mir/wayland/weak.h>
#include <mir/scene/surface.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <random>
#include <sstream>
#include <iomanip>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace ms = mir::scene;
namespace msh = mir::shell;

namespace
{
std::string generate_handle()
{
    static std::mt19937_64 gen{std::random_device{}()};
    static std::uniform_int_distribution<uint64_t> dist;
    static std::mutex gen_mutex;

    std::lock_guard lock{gen_mutex};
    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(16) << dist(gen)
       << std::setw(16) << dist(gen);
    return ss.str();
}

class ZxdgImportedV2;

/// Registry of exported surface handles shared between exporter and importer globals.
///
/// Maps handle strings to exported WlSurface references, and tracks which importers
/// are watching each handle so they can be notified when the export is destroyed.
class XdgForeignV2Registry
{
public:
    struct ExportEntry
    {
        mw::Weak<mf::WlSurface> surface;
        mw::DestroyListenerId surface_destroy_listener;
        std::vector<ZxdgImportedV2*> importers;
    };

    /// Create a new export entry and return the handle string.
    std::string add_export(mf::WlSurface* surface, std::function<void()> on_surface_destroyed)
    {
        std::lock_guard lock{mutex};
        auto handle = generate_handle();
        auto& entry = exports[handle];
        entry.surface = mw::make_weak(surface);
        entry.surface_destroy_listener =
            surface->add_destroy_listener(std::move(on_surface_destroyed));
        return handle;
    }

    /// Remove an export entry (called when the exported object is destroyed).
    void remove_export(std::string const& handle)
    {
        std::lock_guard lock{mutex};
        exports.erase(handle);
    }

    /// Look up the exported surface for a handle.
    std::optional<mw::Weak<mf::WlSurface>> get_export(std::string const& handle)
    {
        std::lock_guard lock{mutex};
        auto it = exports.find(handle);
        if (it == exports.end())
            return std::nullopt;
        return it->second.surface;
    }

    /// Register an importer as interested in a handle.
    void add_importer(std::string const& handle, ZxdgImportedV2* importer)
    {
        std::lock_guard lock{mutex};
        auto it = exports.find(handle);
        if (it != exports.end())
            it->second.importers.push_back(importer);
    }

    /// Unregister an importer from a handle.
    void remove_importer(std::string const& handle, ZxdgImportedV2* importer)
    {
        std::lock_guard lock{mutex};
        auto it = exports.find(handle);
        if (it == exports.end())
            return;
        auto& importers = it->second.importers;
        importers.erase(std::remove(importers.begin(), importers.end(), importer), importers.end());
    }

    /// Notify all importers for a handle that the exported surface is gone.
    /// Returns the list of importers to notify (after removing them from the registry).
    std::vector<ZxdgImportedV2*> take_importers(std::string const& handle)
    {
        std::lock_guard lock{mutex};
        auto it = exports.find(handle);
        if (it == exports.end())
            return {};
        std::vector<ZxdgImportedV2*> result;
        result.swap(it->second.importers);
        return result;
    }

    std::mutex mutex;
    std::unordered_map<std::string, ExportEntry> exports;
};

/// Per-client importer instance.
class ZxdgImportedV2 : public mw::XdgImportedV2
{
public:
    ZxdgImportedV2(
        wl_resource* resource,
        std::string handle,
        std::shared_ptr<XdgForeignV2Registry> const& registry)
        : mw::XdgImportedV2{resource, Version<1>()},
          handle{std::move(handle)},
          registry{registry}
    {
        registry->add_importer(this->handle, this);
    }

    ~ZxdgImportedV2() override
    {
        registry->remove_importer(handle, this);
    }

    /// Called when the exported surface is destroyed.
    void notify_destroyed()
    {
        registry->remove_importer(handle, this);
        send_destroyed_event();
    }

private:
    void set_parent_of(struct wl_resource* surface) override
    {
        auto* child = mf::WlSurface::from(surface);
        if (!child)
            return;

        auto exported = registry->get_export(handle);
        if (!exported)
            return;

        if (!*exported)
            return;

        auto& exported_surface = exported->value();
        if (auto scene_surf = exported_surface.scene_surface())
        {
            msh::SurfaceSpecification spec;
            spec.parent = scene_surf.value();
            child->update_surface_spec(spec);
        }
        else
        {
            // Defer setting the parent until the exported surface has a scene surface.
            mw::Weak<mf::WlSurface> weak_child{child};
            exported_surface.on_scene_surface_created(
                [weak_child](std::shared_ptr<ms::Surface> scene_surf)
                {
                    if (weak_child)
                    {
                        msh::SurfaceSpecification spec;
                        spec.parent = std::move(scene_surf);
                        weak_child.value().update_surface_spec(spec);
                    }
                });
        }
    }

    std::string const handle;
    std::shared_ptr<XdgForeignV2Registry> const registry;
};

/// Per-export object: holds the handle and sends it to the client.
class ZxdgExportedV2 : public mw::XdgExportedV2
{
public:
    ZxdgExportedV2(
        wl_resource* resource,
        mf::WlSurface* surface,
        std::shared_ptr<XdgForeignV2Registry> const& registry,
        std::shared_ptr<mir::Executor> const& wayland_executor)
        : mw::XdgExportedV2{resource, Version<1>()},
          registry{registry},
          wayland_executor{wayland_executor}
    {
        mw::Weak<ZxdgExportedV2> weak_self{this};
        handle = registry->add_export(
            surface,
            [weak_self, wayland_executor = this->wayland_executor]()
            {
                wayland_executor->spawn(
                    [weak_self]()
                    {
                        if (weak_self)
                        {
                            weak_self.value().on_surface_destroyed();
                        }
                    });
            });

        send_handle_event(handle);
    }

    ~ZxdgExportedV2() override
    {
        // Notify importers that the export has been revoked (either by explicit destroy
        // or by the surface being destroyed).
        auto importers = registry->take_importers(handle);
        for (auto* importer : importers)
        {
            importer->notify_destroyed();
        }
        registry->remove_export(handle);
    }

private:
    void on_surface_destroyed()
    {
        auto importers = registry->take_importers(handle);
        for (auto* importer : importers)
        {
            importer->notify_destroyed();
        }
        registry->remove_export(handle);
    }

    std::string handle;
    std::shared_ptr<XdgForeignV2Registry> const registry;
    std::shared_ptr<mir::Executor> const wayland_executor;
};

/// Per-client exporter instance.
class ZxdgExporterV2 : public mw::XdgExporterV2
{
public:
    ZxdgExporterV2(
        wl_resource* resource,
        std::shared_ptr<XdgForeignV2Registry> const& registry,
        std::shared_ptr<mir::Executor> const& wayland_executor)
        : mw::XdgExporterV2{resource, Version<1>()},
          registry{registry},
          wayland_executor{wayland_executor}
    {
    }

private:
    void export_toplevel(struct wl_resource* id, struct wl_resource* surface) override
    {
        auto* wl_surface = mf::WlSurface::from(surface);
        if (!wl_surface)
            return;
        new ZxdgExportedV2{id, wl_surface, registry, wayland_executor};
    }

    std::shared_ptr<XdgForeignV2Registry> const registry;
    std::shared_ptr<mir::Executor> const wayland_executor;
};

/// Per-client importer instance (manager object).
class ZxdgImporterV2 : public mw::XdgImporterV2
{
public:
    ZxdgImporterV2(
        wl_resource* resource,
        std::shared_ptr<XdgForeignV2Registry> const& registry)
        : mw::XdgImporterV2{resource, Version<1>()},
          registry{registry}
    {
    }

private:
    void import_toplevel(struct wl_resource* id, std::string const& handle) override
    {
        new ZxdgImportedV2{id, handle, registry};
    }

    std::shared_ptr<XdgForeignV2Registry> const registry;
};

/// The xdg_foreign_unstable_v2 implementation. Holds two globals (exporter and importer)
/// sharing a single registry.
class XdgForeignV2
{
public:
    XdgForeignV2(
        wl_display* display,
        std::shared_ptr<mir::Executor> const& wayland_executor)
        : registry{std::make_shared<XdgForeignV2Registry>()},
          exporter_global{std::make_shared<ExporterGlobal>(display, registry, wayland_executor)},
          importer_global{std::make_shared<ImporterGlobal>(display, registry)}
    {
    }

private:
    class ExporterGlobal : public mw::XdgExporterV2::Global
    {
    public:
        ExporterGlobal(
            wl_display* display,
            std::shared_ptr<XdgForeignV2Registry> const& registry,
            std::shared_ptr<mir::Executor> const& wayland_executor)
            : mw::XdgExporterV2::Global{display, Version<1>()},
              registry{registry},
              wayland_executor{wayland_executor}
        {
        }

    private:
        void bind(wl_resource* new_resource) override
        {
            new ZxdgExporterV2{new_resource, registry, wayland_executor};
        }

        std::shared_ptr<XdgForeignV2Registry> const registry;
        std::shared_ptr<mir::Executor> const wayland_executor;
    };

    class ImporterGlobal : public mw::XdgImporterV2::Global
    {
    public:
        ImporterGlobal(
            wl_display* display,
            std::shared_ptr<XdgForeignV2Registry> const& registry)
            : mw::XdgImporterV2::Global{display, Version<1>()},
              registry{registry}
        {
        }

    private:
        void bind(wl_resource* new_resource) override
        {
            new ZxdgImporterV2{new_resource, registry};
        }

        std::shared_ptr<XdgForeignV2Registry> const registry;
    };

    std::shared_ptr<XdgForeignV2Registry> const registry;
    std::shared_ptr<ExporterGlobal> const exporter_global;
    std::shared_ptr<ImporterGlobal> const importer_global;
};
}

auto mf::create_xdg_foreign_unstable_v2(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor)
    -> std::shared_ptr<void>
{
    return std::make_shared<XdgForeignV2>(display, wayland_executor);
}
