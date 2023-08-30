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

#include "miral/smooth_boot_support.h"

#include <mir/server.h>
#include <mir/options/configuration.h>
#include <mir/graphics/display.h>
#include <mir/graphics/display_buffer.h>

namespace mg = mir::graphics;
namespace mo = mir::options;

struct miral::SmoothBootSupport::Self
{
};

miral::SmoothBootSupport::SmoothBootSupport()
    : self{std::make_shared<Self>()}
{
}

miral::SmoothBootSupport::~SmoothBootSupport() = default;
miral::SmoothBootSupport::SmoothBootSupport(SmoothBootSupport const& other) = default;
auto miral::SmoothBootSupport::operator=(miral::SmoothBootSupport const& other) -> SmoothBootSupport& = default;

void miral::SmoothBootSupport::operator()(mir::Server &server) const
{
    server.add_init_callback([&server]()
    {
        server.the_display()->for_each_display_sync_group([&server](mg::DisplaySyncGroup& group)
        {
            group.for_each_display_buffer([&server](mg::DisplayBuffer& db)
            {
                db.view_area();
            });
        });
    });
    server.add_configuration_option(
        mo::smooth_boot_opt,
        "When set, provides a transition from the previous screen to the compositor",
        true
    );
}