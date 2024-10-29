/*
 *
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

#include "miral/decoration/decoration_manager_adapter.h"
#include <memory>

namespace md = miral::decoration;

md::DecorationManagerAdapter::DecorationManagerAdapter() = default;

void md::DecorationManagerAdapter::init(std::weak_ptr<mir::shell::Shell> const& shell)
{
    on_init(shell);
}

void md::DecorationManagerAdapter::decorate(std::shared_ptr<mir::scene::Surface> const& surface)
{
    on_decorate(surface);
}

void md::DecorationManagerAdapter::undecorate(std::shared_ptr<mir::scene::Surface> const& surface)
{
    on_undecorate(surface);
}

void md::DecorationManagerAdapter::undecorate_all()
{
    on_undecorate_all();
}

md::DecorationManagerBuilder::DecorationManagerBuilder() :
    adapter{std::unique_ptr<DecorationManagerAdapter>(new DecorationManagerAdapter{})}
{
}

auto md::DecorationManagerBuilder::build(
        std::function<void(std::weak_ptr<mir::shell::Shell> const& shell)> on_init,
        std::function<void(std::shared_ptr<mir::scene::Surface> const& surface)> on_decorate,
        std::function<void(std::shared_ptr<mir::scene::Surface> const& surface)> on_undecorate,
        std::function<void()> on_undecorate_all) -> DecorationManagerBuilder
{
    auto builder = DecorationManagerBuilder{};
    auto& adapter = builder.adapter;

    adapter->on_init = on_init;
    adapter->on_decorate = on_decorate;
    adapter->on_undecorate = on_undecorate;
    adapter->on_undecorate_all = on_undecorate_all;

    return builder;
}


auto md::DecorationManagerBuilder::done() -> std::shared_ptr<DecorationManagerAdapter>
{
    return std::move(adapter);
}
