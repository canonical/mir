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

#include <miral/decorations.h>
#include "decoration_strategy_adapter.h"

#include <mir/decoration_strategy.h>
#include <mir/fatal.h>
#include <mir/server.h>

#include <memory>

struct miral::Decorations::Self : mir::DecorationStrategy
{
};

void miral::Decorations::operator()(mir::Server& server) const
{
    server.add_pre_init_callback([this, &server]() { server.set_the_decoration_strategy(self); });
}

miral::Decorations::Decorations(std::shared_ptr<Self> strategy) : self{std::move(strategy)} {}

auto miral::Decorations::always_ssd() -> Decorations
{
    struct AlwaysServerSide : Self
    {
        DecorationsType default_style() const override { return DecorationsType::ssd; }
        DecorationsType request_style(DecorationsType) const override { return DecorationsType::ssd; }
    };

    return Decorations(std::make_shared<AlwaysServerSide>());
}

auto miral::Decorations::always_csd() -> Decorations
{
    struct AlwaysClientSide : Self
    {
        DecorationsType default_style() const override { return DecorationsType::csd; }
        DecorationsType request_style(DecorationsType) const override { return DecorationsType::csd; }
    };

    return Decorations(std::make_shared<AlwaysClientSide>());
}

auto miral::Decorations::prefer_ssd() -> Decorations
{
    struct PreferServerSide : Self
    {
        DecorationsType default_style() const override { return DecorationsType::ssd; }
        DecorationsType request_style(DecorationsType type) const override { return type; }
    };

    return Decorations(std::make_shared<PreferServerSide>());
}

auto miral::Decorations::prefer_csd() -> Decorations
{
    struct PreferClientSide : Self
    {
        DecorationsType default_style() const override { return DecorationsType::csd; }
        DecorationsType request_style(DecorationsType type) const override { return type; }
    };

    return Decorations(std::make_shared<PreferClientSide>());
}

struct miral::CustomDecorations::Self
{
    std::shared_ptr<miral::DecorationStrategy> strategy;
};

miral::CustomDecorations::CustomDecorations(std::shared_ptr<Self> self) : self{std::move(self)} {}

miral::CustomDecorations::CustomDecorations() : self{std::make_shared<Self>()} {}

auto miral::CustomDecorations::strategy(std::shared_ptr<miral::DecorationStrategy> custom) -> CustomDecorations
{
    if (!custom)
        mir::fatal_error("CustomDecorations::strategy(): null DecorationStrategy passed to factory");

    auto self = std::make_shared<Self>();
    self->strategy = std::move(custom);
    return CustomDecorations(self);
}

void miral::CustomDecorations::operator()(mir::Server& server) const
{
    if (!self || !self->strategy)
        return;

    auto const keep_alive = self;
    server.add_pre_init_callback(
        [keep_alive, &server]()
        {
            if (!keep_alive || !keep_alive->strategy)
                mir::fatal_error("CustomDecorations: strategy became null before init callback");

            auto const allocator = server.the_buffer_allocator(); // keep_alive + strong ref in adapter guarantees lifetime for pre-init/shutdown safety
            if (!allocator)
                mir::fatal_error("CustomDecorations: no buffer allocator available");

            auto adapter = std::make_shared<miral::DecorationStrategyAdapter>(keep_alive->strategy, allocator);
            server.set_the_decoration_renderer_strategy(adapter);
        });
}
