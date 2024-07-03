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

#include "miral/decorations.h"

#include <memory>

#include <mir/default_server_configuration.h>
#include <mir/server.h>
#include <mir/decoration_strategy.h>


struct miral::Decorations::Self : mir::DecorationStrategy
{
};

void miral::Decorations::operator()(mir::Server& server) const
{
    server.add_pre_init_callback(
        [this, &server]()
        {
            server.set_the_decoration_strategy(self);
        });
}

miral::Decorations::Decorations(std::shared_ptr<Self> strategy) :
    self{std::move(strategy)}
{
}

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
