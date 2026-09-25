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

#include <miral/output_configuration.h>

#include <mir/graphics/display_configuration_policy.h>
#include <mir/log.h>
#include <mir/logging/tag.h>
#include <mir/shell/display_configuration_controller.h>
#include <mir/server.h>

#include <mutex>
#include <vector>

namespace mg = mir::graphics;
namespace ms = mir::shell;

namespace
{
static mir::logging::Tag const& output_config_tag =
mir::logging::create_tag(mir::logging::graphics(), "output-config");

auto power_mode_name(MirPowerMode power_mode) -> char const*
{
    switch (power_mode)
    {
    case mir_power_mode_on:      return "on";
    case mir_power_mode_standby: return "standby";
    case mir_power_mode_suspend: return "suspend";
    case mir_power_mode_off:     return "off";
    default:                     return "unknown";
    }
}

void log_configuration(char const* source, std::span<mg::UserDisplayConfigurationOutput const> outputs)
{
    if (outputs.empty())
    {
        mir::log_debug({output_config_tag}, "{}: no outputs configured", source);
        return;
    }

    for (auto const& output : outputs)
    {
        if (!output.used)
        {
            mir::log_debug(
                {output_config_tag},
                "{}: output {}: unused (connected {}, power {})",
                source,
                output.name,
                output.connected,
                power_mode_name(output.power_mode));
            continue;
        }

        auto const& mode = output.modes[output.current_mode_index];
        auto const extents = output.extents();

        mir::log_debug(
            {output_config_tag},
            "{}: output {}: used (connected {}, power {}), mode {}x{}@{:.2f}Hz, scale {}, rotation {} degrees, "
            "extents {}x{}+{}+{}",
            source,
            output.name,
            output.connected,
            power_mode_name(output.power_mode),
            mode.size.width.as_int(),
            mode.size.height.as_int(),
            mode.vrefresh_hz,
            output.scale,
            static_cast<int>(output.orientation),
            extents.size.width.as_int(),
            extents.size.height.as_int(),
            extents.top_left.x.as_int(),
            extents.top_left.y.as_int());
    }
}
}

class miral::OutputConfiguration::Self : public mg::DisplayConfigurationPolicy
{
public:
    Self(std::shared_ptr<Strategy> strategy)
        : strategy{std::move(strategy)}
    {
    }

    void set_wrapped(std::shared_ptr<mg::DisplayConfigurationPolicy> wrapped)
    {
        std::lock_guard lock(mutex);
        this->wrapped = std::move(wrapped);
    }

    void set_display_configuration_controller(std::weak_ptr<ms::DisplayConfigurationController> dcc)
    {
        std::lock_guard lock(mutex);
        dcc_weak = std::move(dcc);
    }

    void update_strategy(std::shared_ptr<Strategy> strategy)
    {
        std::shared_ptr<ms::DisplayConfigurationController> dcc;

        {
            std::lock_guard lock(mutex);
            this->strategy = std::move(strategy);
            dcc = dcc_weak.lock();
        }

        // Don't hold the lock while reapplying: the controller calls back into apply_to()
        if (dcc)
        {
            auto config = dcc->base_configuration();
            apply_to(*config);
            dcc->set_base_configuration(config);
        }
    }

    void apply_to(mg::DisplayConfiguration& conf) override
    {
        std::lock_guard lock(mutex);

        if (wrapped)
        {
            wrapped->apply_to(conf);
        }

        std::vector<mg::UserDisplayConfigurationOutput> outputs;
        conf.for_each_output([&](mg::UserDisplayConfigurationOutput& output){ outputs.emplace_back(output); });

        log_configuration("before", outputs);
        strategy->apply_configuration(outputs);

        // The outputs have been updated in place, but CompositeDisplayConfiguration only
        // propagates those updates to the per-platform configurations it aggregates while
        // iterating, so we need this otherwise pointless pass.
        conf.for_each_output([](mg::UserDisplayConfigurationOutput&) {});
    }

    void confirm(mg::DisplayConfiguration const& conf) override
    {
        std::lock_guard lock(mutex);
        if (wrapped)
        {
            wrapped->confirm(conf);
        }

        std::vector<mir::graphics::UserDisplayConfigurationOutput> outputs;

        conf.for_each_output([&](mg::DisplayConfigurationOutput const& output)
        {
            // We need to populate the vector with non-const references to the outputs,
            // but we never share non-const references to the outputs outside this function
            outputs.emplace_back(const_cast<mg::DisplayConfigurationOutput&>(output)); //TICS !cppcoreguidelines-pro-type-const-cast
        });

        log_configuration("after ", outputs);
        strategy->confirm_configuration(outputs);
    }

private:
    std::mutex mutex;
    std::shared_ptr<Strategy> strategy;
    std::shared_ptr<mg::DisplayConfigurationPolicy> wrapped{};
    std::weak_ptr<ms::DisplayConfigurationController> dcc_weak{};
};

miral::OutputConfiguration::Strategy::Strategy() = default;
miral::OutputConfiguration::Strategy::~Strategy() = default;

void miral::OutputConfiguration::NullStrategy::apply_configuration(std::span<mg::UserDisplayConfigurationOutput>)
{
    // Do nothing
}

void miral::OutputConfiguration::NullStrategy::confirm_configuration(std::span<mg::UserDisplayConfigurationOutput const>)
{
    // Do nothing
}

miral::OutputConfiguration::NullStrategy::~NullStrategy() = default;

miral::OutputConfiguration::OutputConfiguration()
    : self{std::make_shared<Self>(std::make_shared<NullStrategy>())}
{
}

miral::OutputConfiguration::OutputConfiguration(std::shared_ptr<Strategy> strategy)
    : self{std::make_shared<Self>(std::move(strategy))}
{
}

void miral::OutputConfiguration::operator()(mir::Server& server) const
{
    server.wrap_display_configuration_policy([self=self](auto wrapped)
        {
            self->set_wrapped(std::move(wrapped));
            return self;
        });

    // The controller can only be obtained once the display exists: asking for it while the
    // display configuration policy is being built would re-enter display construction
    server.add_init_callback([self=self, &server]
        {
            self->set_display_configuration_controller(server.the_display_configuration_controller());
        });
}

void miral::OutputConfiguration::update_strategy(std::shared_ptr<Strategy> strategy)
{
    self->update_strategy(std::move(strategy));
}

miral::OutputConfiguration::~OutputConfiguration() = default;
miral::OutputConfiguration::OutputConfiguration(OutputConfiguration const&) = default;
auto miral::OutputConfiguration::operator=(OutputConfiguration const&) -> OutputConfiguration& = default;
