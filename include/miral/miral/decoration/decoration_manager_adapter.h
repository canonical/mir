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


#ifndef MIRAL_DECORATION_DECORATION_MANAGER_ADAPTER_H
#define MIRAL_DECORATION_DECORATION_MANAGER_ADAPTER_H

#include "mir/shell/decoration_manager.h"
#include <algorithm>
#include <functional>
#include <memory>

namespace miral::decoration
{
class DecorationManagerBuilder;
class DecorationManagerAdapter : public mir::shell::decoration::Manager
{
public:
    void init(std::weak_ptr<mir::shell::Shell> const& shell) override;
    void decorate(std::shared_ptr<mir::scene::Surface> const& surface) override;
    void undecorate(std::shared_ptr<mir::scene::Surface> const& surface) override;
    void undecorate_all() override;

private:
    friend DecorationManagerBuilder;
    DecorationManagerAdapter();

    std::function<void(std::weak_ptr<mir::shell::Shell> const& shell)> on_init;
    std::function<void(std::shared_ptr<mir::scene::Surface> const& surface)> on_decorate;
    std::function<void(std::shared_ptr<mir::scene::Surface> const& surface)> on_undecorate;
    std::function<void()> on_undecorate_all;
};

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
    std::shared_ptr<DecorationManagerAdapter> adapter;
};
}

#endif
