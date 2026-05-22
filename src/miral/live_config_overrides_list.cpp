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

#include <mir/log.h>
#include <miral/live_config_overrides_list.h>

namespace mlc = miral::live_config;

mlc::OverridesList::OverridesList(std::unique_ptr<Context> ctx) :
    ctx{std::move(ctx)}
{
}

mlc::OverridesList& mlc::OverridesList::operator=(OverridesList&&) noexcept = default;
mlc::OverridesList::OverridesList(OverridesList&&) noexcept = default;
mlc::OverridesList::~OverridesList() = default;

void mlc::OverridesList::for_each(Loader unchanged, Loader fresh, Loader modified, Dropped dropped) const
{
    for (auto const& event : ctx->events)
    {
        if (event.kind == Context::Kind::dropped)
        {
            dropped(event.path);
            continue;
        }

        // Openers must not throw; they signal failure by returning nullptr.
        auto stream = event.opener(event.path);
        if (!stream || !*stream)
        {
            if (event.kind == Context::Kind::fresh)
                mir::log_warning("Failed to open new file %s. Skipping.", event.path.c_str());
            else
            {
                mir::log_warning("Failed to open file %s. Treating as a drop.", event.path.c_str());
                dropped(event.path);
            }
            continue;
        }

        switch (event.kind)
        {
        case Context::Kind::unchanged: unchanged(event.path, *stream); break;
        case Context::Kind::fresh:     fresh(event.path, *stream);     break;
        case Context::Kind::modified:  modified(event.path, *stream);  break;
        case Context::Kind::dropped:   break;
        }
    }
}
