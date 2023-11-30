/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_FRONTEND_DESKTOP_FILE_MANAGER_H
#define MIR_FRONTEND_DESKTOP_FILE_MANAGER_H

#include "mir/fd.h"
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <gio/gdesktopappinfo.h>

namespace mir
{
class MainLoop;

namespace scene { class Surface; }

namespace frontend
{

struct DesktopFile
{
    DesktopFile(const char* id, const char* wm_class, const char* exec);
    std::string id;
    std::string wm_class;
    std::string exec;
};

/// Provides the DesktopFileManager with the list of files.
class DesktopFileCache
{
public:
    DesktopFileCache() = default;
    virtual ~DesktopFileCache() = default;
    virtual std::shared_ptr<DesktopFile> lookup_by_app_id(std::string const&) const = 0;
    virtual std::shared_ptr<DesktopFile> lookup_by_wm_class(std::string const&) const = 0;
    virtual std::vector<std::shared_ptr<DesktopFile>> const& get_desktop_files() const = 0;
};

class DesktopFileManager
{
public:
    DesktopFileManager(std::shared_ptr<DesktopFileCache> cache);
    ~DesktopFileManager() = default;

    std::string resolve_app_id(const scene::Surface*);
private:
    std::shared_ptr<DesktopFileCache> cache;
    std::shared_ptr<DesktopFile> resolve_from_wayland_app_id(std::string& app_id);
    std::shared_ptr<DesktopFile> lookup_basename(std::string& name);
    std::shared_ptr<DesktopFile> resolve_if_snap(int pid);
    std::shared_ptr<DesktopFile> resolve_if_flatpak(int pid);
    std::shared_ptr<DesktopFile> resolve_if_executable_matches(int pid);
};
}
}

#endif //MIR_DESKTOP_FILE_MANAGER_H
