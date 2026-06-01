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

#include "live_config_overrides_list_builder.h"

#include <miral/live_config_overrides_list.h>

#include <memory>

namespace mlc = miral::live_config;

auto mlc::OverridesListBuilder::push_unchanged(std::filesystem::path filepath, Opener opener) -> OverridesListBuilder&
{
    ctx->events.emplace_back(Context::Kind::unchanged, std::move(filepath), std::move(opener));
    return *this;
}

auto mlc::OverridesListBuilder::push_new(std::filesystem::path filepath, Opener opener) -> OverridesListBuilder&
{
    ctx->events.emplace_back(Context::Kind::fresh, std::move(filepath), std::move(opener));
    return *this;
}

auto mlc::OverridesListBuilder::push_modified(std::filesystem::path filepath, Opener opener) -> OverridesListBuilder&
{
    ctx->events.emplace_back(Context::Kind::modified, std::move(filepath), std::move(opener));
    return *this;
}

auto mlc::OverridesListBuilder::push_dropped(std::filesystem::path filepath) -> OverridesListBuilder&
{
    ctx->events.emplace_back(Context::Kind::dropped, std::move(filepath), Opener{});
    return *this;
}

auto mlc::OverridesListBuilder::build() -> OverridesList
{
    auto tmp = std::make_unique<Context>();
    std::swap(tmp, ctx);
    return OverridesList{std::move(tmp)};
}
