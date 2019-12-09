/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_SHELL_DECORATION_BASIC_MANAGER_H_
#define MIR_SHELL_DECORATION_BASIC_MANAGER_H_

#include "manager.h"
#include <functional>
#include <unordered_map>
#include <mutex>

namespace mir
{
class Executor;
namespace graphics
{
class GraphicBufferAllocator;
}
namespace shell
{
class Shell;
namespace decoration
{
class Decoration;

/// Facilitates decorating windows with Mir's built-in server size decorations
class BasicManager
    : public Manager
{
public:
    using DecorationBuilder = std::function<std::unique_ptr<Decoration>(
        std::shared_ptr<shell::Shell> const& shell,
        std::shared_ptr<scene::Surface> const& surface)>;

    BasicManager(DecorationBuilder&& decoration_builder);
    ~BasicManager();

    void init(std::weak_ptr<shell::Shell> const& shell) override;
    void decorate(std::shared_ptr<scene::Surface> const& surface) override;
    void undecorate(std::shared_ptr<scene::Surface> const& surface) override;
    void undecorate_all() override;

private:
    DecorationBuilder const decoration_builder;
    std::weak_ptr<shell::Shell> shell;

    std::mutex mutex;
    std::unordered_map<scene::Surface*, std::unique_ptr<Decoration>> decorations;
};
}
}
}

#endif // MIR_SHELL_DECORATION_BASIC_MANAGER_H_
