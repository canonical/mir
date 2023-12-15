/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "multiplexing_display.h"
#include "mir/graphics/display_configuration.h"
#include "mir/renderer/gl/context.h"
#include "mir/output_type_names.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <sstream>
#include <functional>

namespace mg = mir::graphics;

mg::MultiplexingDisplay::MultiplexingDisplay(
    std::vector<std::unique_ptr<Display>> displays,
    DisplayConfigurationPolicy& initial_configuration_policy)
    : displays{std::move(displays)}
{
    auto conf = configuration();
    initial_configuration_policy.apply_to(*conf);
    initial_configuration_policy.confirm(*conf);
    configure(*conf);
}

mg::MultiplexingDisplay::~MultiplexingDisplay() = default;

void mg::MultiplexingDisplay::for_each_display_sync_group(
    std::function<void(DisplaySyncGroup&)> const& f)
{
    for (auto const& display: displays)
    {
        display->for_each_display_sync_group(f);
    }
}

class CompositeDisplayConfiguration : public mg::DisplayConfiguration
{
public:
    CompositeDisplayConfiguration(std::vector<std::unique_ptr<mg::DisplayConfiguration>> confs)
        : components{std::move(confs)},
          id_map{this->components, configuration}
    {
    }

    void for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> functor) const override
    {
        for (auto const& output : configuration)
        {
            functor(output);
        }
    }

    void for_each_output(std::function<void(mg::UserDisplayConfigurationOutput&)> functor) override
    {
        for (auto const& conf : components)
        {
            conf->for_each_output(
                [this, &functor, component = conf.get()](mg::UserDisplayConfigurationOutput& output)
                {
                    mg::UserDisplayConfigurationOutput mutable_output{
                        id_map.configuration_for_internal_id(component, output.id)
                    };
                    functor(mutable_output);
                    // Copy back any changes to the mutable bits:
                    output.used = mutable_output.used;
                    output.top_left = mutable_output.top_left;
                    output.current_mode_index = mutable_output.current_mode_index;
                    output.current_format = mutable_output.current_format;
                    output.power_mode = mutable_output.power_mode;
                    output.orientation = mutable_output.orientation;
                    output.scale = mutable_output.scale;
                    output.form_factor = mutable_output.form_factor;
                    // MirSubpixelArrangement is mistakenly non-const?
                    output.gamma = mutable_output.gamma;
                    output.custom_logical_size = mutable_output.custom_logical_size;
                    output.custom_attribute = std::move(mutable_output.custom_attribute);
                });
        }
    }

    auto clone() const -> std::unique_ptr<DisplayConfiguration> override
    {
        std::vector<std::unique_ptr<mg::DisplayConfiguration>> cloned_conf;
        for (auto const& conf : components)
        {
            cloned_conf.push_back(conf->clone());
        }
        return std::make_unique<CompositeDisplayConfiguration>(std::move(cloned_conf));
    }

    auto index_for_id(mg::DisplayConfigurationOutputId id) const ->
        std::tuple<size_t, mg::DisplayConfigurationOutputId>
    {
        auto const [component, internal_id] = id_map.configuration_for_external_id(id);
        for (size_t i = 0 ; i < components.size(); ++i)
        {
            if (components[i].get() == component)
            {
                return std::make_pair(i, internal_id);
            }
        }
        // id_map.configuration_for_external_id either returns a reference into
        // the components vector or throws, so the above for loop *must* return.
        BOOST_THROW_EXCEPTION((std::logic_error{"Unreachable condition reached"}));
    }

    // Public so that MultiplexingDisplay can pull the components back out later.
    // TODO: Should configure() be a method on *DisplayConfiguration* rather than Display?
    std::vector<std::unique_ptr<mg::DisplayConfiguration>> const components;
private:
    std::vector<mg::DisplayConfigurationOutput> configuration;
    class OutputIdMapper
    {
    public:
        OutputIdMapper(
            std::vector<std::unique_ptr<mg::DisplayConfiguration>> const& confs,
            std::vector<mg::DisplayConfigurationOutput>& backing_store)
            : backing_store{backing_store},
              output_map{construct_output_map(confs, backing_store)}
        {
        }

        auto configuration_for_external_id(mg::DisplayConfigurationOutputId id) const
            -> std::tuple<mg::DisplayConfiguration*, mg::DisplayConfigurationOutputId>
        {
            auto match = std::find_if(
                output_map.begin(),
                output_map.end(),
                [&](auto const& candidate) -> bool
                {
                    return backing_store[candidate.output_index].id == id;
                });
            if (match == output_map.end())
            {
                BOOST_THROW_EXCEPTION((std::out_of_range{"Attempt to look up non-existant output ID"}));
            }
            return std::make_pair(match->owner, match->internal_id);
        }

        auto configuration_for_internal_id(
            mg::DisplayConfiguration* component,
            mg::DisplayConfigurationOutputId internal_id) const -> mg::DisplayConfigurationOutput&
        {
            auto match = std::find_if(
                output_map.begin(),
                output_map.end(),
                [component, internal_id](auto const& candidate) -> bool
                {
                    return candidate.owner == component && candidate.internal_id == internal_id;
                });
            if (match == output_map.end())
            {
                BOOST_THROW_EXCEPTION((std::out_of_range{"Attempt to look up invalid platform/output id pair"}));
            }
            return backing_store[match->output_index];
        }
    private:
        std::vector<mg::DisplayConfigurationOutput>& backing_store;
        struct OutputInfo
        {
            OutputInfo(
                mg::DisplayConfiguration* owner,
                mg::DisplayConfigurationOutputId internal_id,
                size_t output_index)
                : owner{owner},
                  internal_id{internal_id},
                  output_index{output_index}
            {
            }

            mg::DisplayConfiguration* const owner;
            mg::DisplayConfigurationOutputId const internal_id;
            size_t const output_index;
        };
        std::vector<OutputInfo> const output_map;

        static auto name_for_output(
            mg::DisplayConfigurationOutput const& output,
            std::map<mg::DisplayConfigurationCardId, std::map<mg::DisplayConfigurationOutputType, unsigned>>& output_counts)
            -> std::string
        {
            output_counts[output.card_id][output.type]++;    // The map will default-initialise any as-yet-unseen cards to the empty map
                                                             // and any as-yet-unseen output types to 0.
            std::stringstream name;
            name << mir::output_type_name(static_cast<unsigned>(output.type));
            if (output.card_id.as_value())
            {
                name << "-" << output.card_id.as_value();
            }
            name << "-" << output_counts[output.card_id][output.type];
            return name.str();
        }

        static auto construct_output_map(
            std::vector<std::unique_ptr<mg::DisplayConfiguration>> const& confs,
            std::vector<mg::DisplayConfigurationOutput>& backing_store) -> std::vector<OutputInfo>
        {
            std::vector<OutputInfo> outputs;
            std::map<mg::DisplayConfigurationCardId, std::map<mg::DisplayConfigurationOutputType, unsigned>> output_type_counts;
            int next_id = 1;
            int card_index = 0;   // Really, I want std::views::enumerate here, but that's C++23
            for (auto const& conf : confs)
            {
                conf->for_each_output(
                    [&, component = conf.get()](mg::DisplayConfigurationOutput const& output)
                    {
                        backing_store.push_back(output);
                        backing_store.back().id = mg::DisplayConfigurationOutputId{next_id};
                        backing_store.back().card_id = mg::DisplayConfigurationCardId{card_index};
                        backing_store.back().name = name_for_output(backing_store.back(), output_type_counts);
                        next_id++;
                        outputs.emplace_back(
                            component,
                            output.id,
                            backing_store.size() - 1);
                    });
                ++card_index;
            }
            return outputs;
        }
    };
    OutputIdMapper const id_map;
};

auto mg::MultiplexingDisplay::configuration() const -> std::unique_ptr<DisplayConfiguration>
{
    std::vector<std::unique_ptr<mg::DisplayConfiguration>> cloned_conf;
    for (auto const& display : displays)
    {
        cloned_conf.push_back(display->configuration());
    }
    return std::make_unique<CompositeDisplayConfiguration>(std::move(cloned_conf));
}

auto mg::MultiplexingDisplay::apply_if_configuration_preserves_display_buffers(
    DisplayConfiguration const& conf) -> bool
{
    auto const& real_conf = dynamic_cast<CompositeDisplayConfiguration const&>(conf);
    std::vector<std::pair<mg::Display*, std::unique_ptr<mg::DisplayConfiguration>>> previous_configs;

    for (auto i = 0u; i < displays.size(); ++i)
    {
        previous_configs.push_back(std::make_pair(displays[i].get(), displays[i]->configuration()));
        if (!displays[i]->apply_if_configuration_preserves_display_buffers(*real_conf.components[i]))
        {
            // We don't need to revert the last change; that didn't succeed.
            previous_configs.pop_back();

            for (auto saved_config = previous_configs.crbegin(); saved_config != previous_configs.crend(); ++saved_config)
            {
                if (!saved_config->first->apply_if_configuration_preserves_display_buffers(*saved_config->second))
                {
                    BOOST_THROW_EXCEPTION((
                        mg::Display::IncompleteConfigurationApplied{"Failure attempting to undo partially-applied configuration"}));
                }
            }
            return false;
        }
    }
    return true;
}

void mg::MultiplexingDisplay::configure(DisplayConfiguration const& conf)
{
    auto const& real_conf = dynamic_cast<CompositeDisplayConfiguration const&>(conf);
    for (auto i = 0u; i < displays.size(); ++i)
    {
        displays[i]->configure(*real_conf.components[i]);
    }
}

void mg::MultiplexingDisplay::register_configuration_change_handler(
    EventHandlerRegister& handlers,
    DisplayConfigurationChangeHandler const& conf_change_handler)
{
    /* We don't need to, and can't reasonably, do any aggregation here.
     *
     * The conf_change_handler is called by the Display if/when a hardware change
     * is detected on the hardware it controls; if multiple Displays detect a hardware
     * change the conf_change_handler will be called once per change, but that's not
     * actually different to today.
     */
    for (auto& display : displays)
    {
        display->register_configuration_change_handler(handlers, conf_change_handler);
    }
}

void mg::MultiplexingDisplay::pause()
{
    for (auto& display : displays)
    {
        display->pause();
    }
}

void mg::MultiplexingDisplay::resume()
{
    for (auto& display : displays)
    {
        display->resume();
    }
}

auto mg::MultiplexingDisplay::create_hardware_cursor() -> std::shared_ptr<Cursor>
{
    return {};
}
