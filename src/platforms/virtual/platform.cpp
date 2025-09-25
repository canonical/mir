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
#include "mir/graphics/egl_error.h"
#include "mir/log.h"
#include "options_parsing_helpers.h"
#include <drm_fourcc.h>

namespace mg = mir::graphics;
namespace mgv = mir::graphics::virt;
namespace geom = mir::geometry;
using namespace std::literals;


mgv::Platform::Platform(
    std::shared_ptr<mg::DisplayReport> const& report,
    std::vector<VirtualOutputConfig> outputs)
    : report{report},
      outputs{outputs}
{
}

mgv::Platform::~Platform()
{
}

mir::UniqueModulePtr<mg::Display> mgv::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const&,
    std::shared_ptr<GLConfig> const&)
{
    return mir::make_module_ptr<mgv::Display>(outputs);
}

auto mgv::Platform::maybe_create_provider(DisplayProvider::Tag const& type_tag) -> std::shared_ptr<DisplayProvider>
{
    if (dynamic_cast<CPUAddressableDisplayProvider::Tag const*>(&type_tag))
    {
        return std::make_shared<mg::CPUAddressableDisplayProvider>();
    }
    return nullptr;
}

auto mgv::Platform::parse_output_sizes(std::vector<std::string> virtual_outputs) -> std::vector<mgv::VirtualOutputConfig>
{
    std::vector<VirtualOutputConfig> configs;
    for (auto const& output : virtual_outputs)
    {
        std::vector<geom::Size > sizes;
        for (int start = 0, end; start - 1 < (int)output.size(); start = end + 1)
        {
            end = output.find(':', start);
            if (end == (int)std::string::npos)
                end = output.size();
            sizes.push_back(common::parse_size(output.substr(start, end - start)));
        }

        configs.push_back(VirtualOutputConfig(std::move(sizes)));
    }
    return configs;
}
