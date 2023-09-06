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
#include <mir/graphics/initial_render.h>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mo = mir::options;

namespace
{
const char* to_option(mg::SmoothSupportBehavior behavior)
{
    switch (behavior)
    {
        case mg::SmoothSupportBehavior::fade:
            return "fade";
        default:
            return nullptr;
    }
}
}

struct miral::SmoothBootSupport::Self
{
    mg::SmoothSupportBehavior behavior = mg::SmoothSupportBehavior::fade;
};

miral::SmoothBootSupport::SmoothBootSupport(uint32_t behavior)
    : self{std::make_shared<Self>()}
{
    if (behavior >= static_cast<uint32_t>(mg::SmoothSupportBehavior::count))
        BOOST_THROW_EXCEPTION(std::invalid_argument("Unsupported smooth boot behavior"));

    self->behavior = static_cast<mg::SmoothSupportBehavior>(behavior);
}

miral::SmoothBootSupport::SmoothBootSupport()
    : self{std::make_shared<Self>()}
{
}

miral::SmoothBootSupport::~SmoothBootSupport() = default;
miral::SmoothBootSupport::SmoothBootSupport(SmoothBootSupport const& other) = default;
auto miral::SmoothBootSupport::operator=(miral::SmoothBootSupport const& other) -> SmoothBootSupport& = default;

void miral::SmoothBootSupport::operator()(mir::Server &server) const
{
    auto option = to_option(self->behavior);
    if (!option)
        BOOST_THROW_EXCEPTION(std::logic_error("Undefined smooth boot behavior option"));

    server.add_configuration_option(
        mo::smooth_boot_opt,
        "When set, provides a transition from the previous screen to the compositor",
        option
    );
}