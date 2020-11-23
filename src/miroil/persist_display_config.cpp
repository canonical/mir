/*
 * Copyright © 2016-2020 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "miroil/persist_display_config.h"

#include <mir/graphics/display_configuration_policy.h>
#include <mir/server.h>
#include <mir/version.h>

#if MIR_SERVER_VERSION >= MIR_VERSION_NUMBER(0, 26, 0)
#include <mir/graphics/display_configuration_observer.h>
#include <mir/observer_registrar.h>
#endif

namespace mg = mir::graphics;

namespace
{
struct PersistDisplayConfigPolicy
{
    PersistDisplayConfigPolicy() = default;
    virtual ~PersistDisplayConfigPolicy() = default;
    PersistDisplayConfigPolicy(PersistDisplayConfigPolicy const&) = delete;
    auto operator=(PersistDisplayConfigPolicy const&) -> PersistDisplayConfigPolicy& = delete;

    void apply_to(mg::DisplayConfiguration& conf, mg::DisplayConfigurationPolicy& default_policy);
    void save_config(mg::DisplayConfiguration const& base_conf);
};

struct DisplayConfigurationPolicyAdapter : mg::DisplayConfigurationPolicy
{
    DisplayConfigurationPolicyAdapter(
        std::shared_ptr<PersistDisplayConfigPolicy> const& self,
        std::shared_ptr<mg::DisplayConfigurationPolicy> const& wrapped) :
        self{self}, default_policy{wrapped}
    {}

    void apply_to(mg::DisplayConfiguration& conf) override
    {
        self->apply_to(conf, *default_policy);
    }

    std::shared_ptr<PersistDisplayConfigPolicy> const self;
    std::shared_ptr<mg::DisplayConfigurationPolicy> const default_policy;
};

#if MIR_SERVER_VERSION >= MIR_VERSION_NUMBER(0, 26, 0)
struct DisplayConfigurationObserver : mg::DisplayConfigurationObserver
{
    void initial_configuration(std::shared_ptr<mg::DisplayConfiguration const> const& /*config*/) override {}

    void configuration_applied(std::shared_ptr<mg::DisplayConfiguration const> const& /*config*/) override {}

    void configuration_failed(
        std::shared_ptr<mg::DisplayConfiguration const> const& /*attempted*/,
        std::exception const& /*error*/) override {}

    void catastrophic_configuration_error(
        std::shared_ptr<mg::DisplayConfiguration const> const& /*failed_fallback*/,
        std::exception const& /*error*/) override {}
};
#else
struct DisplayConfigurationObserver { };
#endif
}

struct miroil::PersistDisplayConfig::Self : PersistDisplayConfigPolicy, DisplayConfigurationObserver
{
    Self() = default;
    Self(DisplayConfigurationPolicyWrapper const& custom_wrapper) :
        custom_wrapper{custom_wrapper} {}

    DisplayConfigurationPolicyWrapper const custom_wrapper =
        [](const std::shared_ptr<mg::DisplayConfigurationPolicy> &wrapped) { return wrapped; };

#if MIR_SERVER_VERSION >= MIR_VERSION_NUMBER(0, 26, 0)
    void base_configuration_updated(std::shared_ptr<mg::DisplayConfiguration const> const& base_config) override
    {
        save_config(*base_config);
    }

    void session_configuration_applied(std::shared_ptr<mir::scene::Session> const&,
                                       std::shared_ptr<mg::DisplayConfiguration> const&){}
    void session_configuration_removed(std::shared_ptr<mir::scene::Session> const&)  {}
    void configuration_updated_for_session(
        std::shared_ptr<mir::scene::Session> const&,
        std::shared_ptr<mg::DisplayConfiguration const> const&) {}
#endif
};

miroil::PersistDisplayConfig::PersistDisplayConfig() :
    self{std::make_shared<Self>()}
{
}

miroil::PersistDisplayConfig::PersistDisplayConfig(DisplayConfigurationPolicyWrapper const& custom_wrapper) :
    self{std::make_shared<Self>(custom_wrapper)}
{
}

miroil::PersistDisplayConfig::~PersistDisplayConfig() = default;

miroil::PersistDisplayConfig::PersistDisplayConfig(PersistDisplayConfig const&) = default;

auto miroil::PersistDisplayConfig::operator=(PersistDisplayConfig const&) -> PersistDisplayConfig& = default;

void miroil::PersistDisplayConfig::operator()(mir::Server& server)
{
    server.wrap_display_configuration_policy(
        [this](std::shared_ptr<mg::DisplayConfigurationPolicy> const& wrapped)
        -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            return std::make_shared<DisplayConfigurationPolicyAdapter>(self, self->custom_wrapper(wrapped));
        });

#if MIR_SERVER_VERSION >= MIR_VERSION_NUMBER(0, 26, 0)
    server.add_init_callback([this, &server]
        { server.the_display_configuration_observer_registrar()->register_interest(self); });
#else
    // Up to Mir-0.25 detecting changes to the base display config is only possible client-side
    // (and gives a different configuration API)
    // If we decide implementing this is necessary for earlier Mir versions then this is where to plumb it in.
    (void)&PersistDisplayConfigPolicy::save_config; // Fake "using" the function for now
#endif
}

void PersistDisplayConfigPolicy::apply_to(
    mg::DisplayConfiguration& conf,
    mg::DisplayConfigurationPolicy& default_policy)
{
    // TODO if the h/w profile (by some definition) has changed, then apply corresponding saved config (if any).
    // TODO Otherwise...
    default_policy.apply_to(conf);
}

void PersistDisplayConfigPolicy::save_config(mg::DisplayConfiguration const& /*base_conf*/)
{
    // TODO save display config options against the h/w profile
}
