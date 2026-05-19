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

mlc::OverridesList::~OverridesList() = default;

void mlc::OverridesList::for_each(Loader unchanged, Loader fresh, Loader modified, Dropped dropped) const
{
    auto const callback_or_drop = [&](auto& event, auto& callback)
    {
        std::unique_ptr<std::istream> stream;
        try
        {
            event.file_opener(callback);
        }
        catch (std::exception const& e)
        {
            mir::log_warning("Error opening file %s: %s. Treating as a drop.", event.path.c_str(), e.what());
            dropped(event.path);
            return;
        }
    };

    for (auto const& event : ctx->events)
    {
        switch (event.kind)
        {
        case OverridesList::Context::Kind::unchanged:
            {
                callback_or_drop(event, unchanged);
                break;
            }
        case OverridesList::Context::Kind::fresh:
            {
                try
                {
                    event.file_opener(fresh);
                }
                catch (std::exception const& e)
                {
                    mir::log_warning("Error opening file %s: %s. Skipping.", event.path.c_str(), e.what());
                }

                break;
            }
        case OverridesList::Context::Kind::modified:
            {
                callback_or_drop(event, modified);
                break;
            }
        case OverridesList::Context::Kind::dropped:
            dropped(event.path);
            break;
        }
    }
}
