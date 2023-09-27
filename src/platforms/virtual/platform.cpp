/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "platform.h"
#include "display.h"
#include "mir/graphics/platform.h"

namespace mg = mir::graphics;
namespace mgv = mir::graphics::virt;
using namespace std::literals;

mgv::Platform::Platform(std::shared_ptr<mg::DisplayReport> const& report) :
    report{report}
{
}

mir::UniqueModulePtr<mg::Display> mgv::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const&,
    std::shared_ptr<GLConfig> const&)
{
    return mir::make_module_ptr<mgv::Display>();
}

auto mgv::Platform::interface_for() -> std::shared_ptr<DisplayInterfaceProvider>
{
    return nullptr; 
}

