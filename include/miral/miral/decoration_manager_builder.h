/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIRAL_DECORATION_MANAGER_BUILDER_H
#define MIRAL_DECORATION_MANAGER_BUILDER_H

#include <functional>
#include <memory>

namespace mir
{
namespace shell
{
class Shell;
}
namespace scene
{
class Surface;
}
}

namespace miral
{

class DecorationManagerAdapter;
class DecorationManagerBuilder
{
public:
    static auto build(
        std::function<void(std::weak_ptr<mir::shell::Shell> const& shell)> on_init,
        std::function<void(std::shared_ptr<mir::scene::Surface> const& surface)> on_decorate,
        std::function<void(std::shared_ptr<mir::scene::Surface> const& surface)> on_undecorate,
        std::function<void()> on_undecorate_all) -> DecorationManagerBuilder;

    auto done() -> std::shared_ptr<DecorationManagerAdapter>;
private:
    DecorationManagerBuilder();

    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif
