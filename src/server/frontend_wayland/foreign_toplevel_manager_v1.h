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

#ifndef MIR_FRONTEND_FOREIGN_TOPLEVEL_MANAGER_V1_H
#define MIR_FRONTEND_FOREIGN_TOPLEVEL_MANAGER_V1_H

#include "wlr-foreign-toplevel-management-unstable-v1_wrapper.h"
#include "desktop_file_manager.h"

#include <memory>
#include <mutex>

namespace mir
{
class Executor;
class MainLoop;
namespace shell
{
class Shell;
}
namespace frontend
{
class SurfaceStack;

auto create_foreign_toplevel_manager_v1(
    wl_display* display,
    std::shared_ptr<shell::Shell> const& shell,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<SurfaceStack> const& surface_stack,
    std::shared_ptr<DesktopFileManager> const& desktop_file_manager)
-> std::shared_ptr<wayland::ForeignToplevelManagerV1::Global>;

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
    mutable std::recursive_mutex update_mutex;
    std::shared_ptr<MainLoop> const main_loop;
    mir::Fd inotify_fd;
    std::vector<int> config_path_wd_list;
    std::vector<std::shared_ptr<DesktopFile>> files;
    std::map<std::string, std::shared_ptr<DesktopFile>> id_to_app;
    std::map<std::string, std::string> wm_class_to_app_info_id;
};

}
}

#endif // MIR_FRONTEND_FOREIGN_TOPLEVEL_MANAGER_V1_H
