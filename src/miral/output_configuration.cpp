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

namespace mg = mir::graphics;
namespace ms = mir::shell;

class miral::OutputConfiguration::Self : public mg::DisplayConfigurationPolicy
{
public:
    Self(std::shared_ptr<Strategy> strategy)
        : strategy{std::move(strategy)}
    {
    }

    void init(mir::Server& server, std::shared_ptr<mg::DisplayConfigurationPolicy> wrapped)
    {
        std::lock_guard lock(mutex);
        this->wrapped = wrapped;
        dcc_weak = server.the_display_configuration_controller();
    }

    void update_strategy(std::shared_ptr<Strategy> strategy)
    {
        std::lock_guard lock(mutex);
        this->strategy = std::move(strategy);
        if (auto dcc = dcc_weak.lock())
        {
            auto config = dcc->base_configuration();
            apply_to_locked(*config);
            dcc->set_base_configuration(config);
        }
    }

    void apply_to(mg::DisplayConfiguration& conf) override
    {
        std::lock_guard lock(mutex);
        apply_to_locked(conf);
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
    void apply_to_locked(mg::DisplayConfiguration& conf)
    {
        if (wrapped)
        {
            wrapped->apply_to(conf);
        }

        std::vector<mir::graphics::UserDisplayConfigurationOutput> outputs;

        conf.for_each_output([&](mg::UserDisplayConfigurationOutput& output)
        {
            outputs.emplace_back(output);
        });

        strategy->apply_configuration(outputs);
    }

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
    server.wrap_display_configuration_policy([self=self,&server](auto wrapped)
        {
            self->init(server, wrapped);
            return self;
        });
}

void miral::OutputConfiguration::update_strategy(std::shared_ptr<Strategy> strategy)
{
    self->update_strategy(std::move(strategy));
}

miral::OutputConfiguration::~OutputConfiguration() = default;
miral::OutputConfiguration::OutputConfiguration(OutputConfiguration const&) = default;
auto miral::OutputConfiguration::operator=(OutputConfiguration const&) -> OutputConfiguration& = default;
