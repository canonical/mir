/*
 * Copyright © 2022 Canonical Ltd.
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
#include <functional>

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;

mg::MultiplexingDisplay::MultiplexingDisplay(std::vector<std::unique_ptr<Display>> displays)
    : displays{std::move(displays)}
{
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

namespace
{
class CompositeDisplayConfiguration : public mg::DisplayConfiguration
{
public:
    CompositeDisplayConfiguration(std::vector<std::unique_ptr<mg::DisplayConfiguration>> confs)
        : components{std::move(confs)}
    {
    }

    void for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> functor) const override
    {
        for (auto const& conf : components)
        {
            conf->for_each_output(functor);
        }
    }

    void for_each_output(std::function<void(mg::UserDisplayConfigurationOutput&)> functor) override
    {
        for (auto const& conf : components)
        {
            conf->for_each_output(functor);
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

    // Public so that MultiplexingDisplay can pull the components back out later.
    // TODO: Should configure() be a method on *DisplayConfiguration* rather than Display?
    std::vector<std::unique_ptr<mg::DisplayConfiguration>> const components;
};
}

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

            for (auto i = previous_configs.crbegin() ; i != previous_configs.crend(); ++i)
            {
                if (!i->first->apply_if_configuration_preserves_display_buffers(*i->second))
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
}

void mg::MultiplexingDisplay::resume()
{
}

auto mg::MultiplexingDisplay::create_hardware_cursor() -> std::shared_ptr<Cursor>
{
    return {};
}

auto mg::MultiplexingDisplay::last_frame_on(unsigned /*output_id*/) const -> Frame
{
    return {};
}

auto mg::MultiplexingDisplay::create_gl_context() const -> std::unique_ptr<renderer::gl::Context>
{
    // This will disappear in the New Platform API™; just return the first underlying Display's context for now.
    return displays[0]->create_gl_context();
}
