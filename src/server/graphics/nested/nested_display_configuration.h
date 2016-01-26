/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Eleni Maria Stea <elenimaria.stea@canonical.com>
 */

#ifndef MIR_GRAPHICS_NESTED_NESTED_DISPLAY_CONFIGURATION_H_
#define MIR_GRAPHICS_NESTED_NESTED_DISPLAY_CONFIGURATION_H_

#include "mir/graphics/display_configuration.h"
#include "mir_toolkit/client_types.h"
#include <memory>
#include <mutex>
#include <unordered_map>

namespace mir
{
namespace graphics
{
namespace nested
{
class NestedDisplayConfiguration : public DisplayConfiguration
{
public:
    NestedDisplayConfiguration(
        std::shared_ptr<MirDisplayConfiguration> const& display_config);
    NestedDisplayConfiguration(NestedDisplayConfiguration const&);

    void for_each_card(std::function<void(DisplayConfigurationCard const&)>) const override;
    void for_each_output(std::function<void(DisplayConfigurationOutput const&)>) const override;
    void for_each_output(std::function<void(UserDisplayConfigurationOutput&)>) override;
    std::unique_ptr<DisplayConfiguration> clone() const override;

    operator MirDisplayConfiguration*() const { return display_config.get(); }

private:
    std::shared_ptr<MirDisplayConfiguration> display_config;

    /*
     * The client display config doesn't currently expose the form factor or scaling factor, nor is it
     * entirely clear that it should allow a client to set them.
     *
     * We therefore need to store these explicitly in the NestedConfiguration.
     */
    std::mutex mutable local_config_mutex;
    struct LocalOutputConfig
    {
        float scale;
        MirFormFactor form_factor;
    };
    std::unordered_map<uint32_t, LocalOutputConfig> mutable local_config;

    LocalOutputConfig get_local_config_for(uint32_t output_id) const;
    void set_local_config_for(uint32_t output_id, LocalOutputConfig const& config);
};

}
}
}

#endif // MIR_GRAPHICS_NESTED_NESTED_DISPLAY_CONFIGURATION_H_
