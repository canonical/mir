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
#include <mir/shell/display_configuration_controller.h>
#include <mir/server.h>

#include <algorithm>
#include <vector>

namespace mg = mir::graphics;
namespace ms = mir::shell;

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

        // The strategy gets to see all the outputs at once, so work on a snapshot...
        std::vector<mg::DisplayConfigurationOutput> snapshot;
        conf.for_each_output([&](mg::DisplayConfigurationOutput const& output) { snapshot.push_back(output); });

        std::vector<mg::UserDisplayConfigurationOutput> outputs;
        outputs.reserve(snapshot.size());
        for (auto& output : snapshot)
        {
            outputs.emplace_back(output);
        }

        strategy->apply_configuration(outputs);

        // ...and write the result back one output at a time, as some DisplayConfiguration
        // implementations only propagate changes made during the for_each_output() callback
        conf.for_each_output([&](mg::UserDisplayConfigurationOutput& output)
            {
                auto const updated = std::ranges::find(snapshot, output.id, &mg::DisplayConfigurationOutput::id);

                if (updated == snapshot.end())
                    return;

                output.logical_group_id = updated->logical_group_id;
                output.used = updated->used;
                output.top_left = updated->top_left;
                output.current_mode_index = updated->current_mode_index;
                output.current_format = updated->current_format;
                output.power_mode = updated->power_mode;
                output.orientation = updated->orientation;
                output.scale = updated->scale;
                output.form_factor = updated->form_factor;
                output.subpixel_arrangement = updated->subpixel_arrangement;
                output.gamma = updated->gamma;
                output.custom_logical_size = updated->custom_logical_size;
                output.custom_attribute = updated->custom_attribute;
            });
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
            outputs.emplace_back(const_cast<mg::DisplayConfigurationOutput&>(output));
        });

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
