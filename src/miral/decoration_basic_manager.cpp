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

#include "miral/decoration_basic_manager.h"
#include "miral/decoration_manager_builder.h"

#include "mir/shell/decoration/basic_manager.h"
#include "mir/shell/decoration.h"
#include "decoration_adapter.h"

#include <memory>
#include <utility>

struct miral::DecorationBasicManager::Self : public mir::shell::decoration::BasicManager
{
    Self(Decoration::DecorationBuilder decoration_builder) :
        mir::shell::decoration::BasicManager(decoration_builder)
    {
    }
};

miral::DecorationBasicManager::DecorationBasicManager(Decoration::DecorationBuilder decoration_builder)
    : self{std::make_shared<Self>(decoration_builder)}
{
}

auto miral::DecorationBasicManager::to_adapter() -> std::shared_ptr<DecorationManagerAdapter>
{
    return DecorationManagerBuilder::build(
        [this](auto... args) { self->init(args...); },
        [this](auto... args) { self->decorate(args...); },
        [this](auto... args) { self->undecorate(args...); },
        [this]() { self->undecorate_all(); }
    ).done();
}
