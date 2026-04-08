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

#ifndef MIRAL_CONFIG_AGGREGATOR_H
#define MIRAL_CONFIG_AGGREGATOR_H

#include "live_config.h"
#include <filesystem>

namespace miral::live_config
{
class ParsingStore;
class ConfigAggregator : public live_config::Store
{
public:
    struct Source
    {
        std::shared_ptr<Store> store;
        std::function<void()> load;
        std::filesystem::path file_path;
    };

    ConfigAggregator();
    ~ConfigAggregator();

    void add_source(Source&& source);
    void remove_source(std::filesystem::path const& path);

    void reload_all();

    void add_int_attribute(live_config::Key const& key, std::string_view description, HandleInt handler) override;
    void add_ints_attribute(live_config::Key const& key, std::string_view description, HandleInts handler) override;
    void add_bool_attribute(live_config::Key const& key, std::string_view description, HandleBool handler) override;
    void add_float_attribute(live_config::Key const& key, std::string_view description, HandleFloat handler) override;
    void add_floats_attribute(live_config::Key const& key, std::string_view description, HandleFloats handler) override;
    void add_string_attribute(live_config::Key const& key, std::string_view description, HandleString handler) override;
    void add_strings_attribute(
        live_config::Key const& key, std::string_view description, HandleStrings handler) override;

    void add_int_attribute(
        live_config::Key const& key, std::string_view description, int preset, HandleInt handler) override;
    void add_ints_attribute(
        live_config::Key const& key,
        std::string_view description,
        std::span<int const> preset,
        HandleInts handler) override;
    void add_bool_attribute(
        live_config::Key const& key, std::string_view description, bool preset, HandleBool handler) override;
    void add_float_attribute(
        live_config::Key const& key, std::string_view description, float preset, HandleFloat handler) override;
    void add_floats_attribute(
        live_config::Key const& key,
        std::string_view description,
        std::span<float const> preset,
        HandleFloats handler) override;
    void add_string_attribute(
        live_config::Key const& key,
        std::string_view description,
        std::string_view preset,
        HandleString handler) override;
    void add_strings_attribute(
        live_config::Key const& key,
        std::string_view description,
        std::span<std::string const> preset,
        HandleStrings handler) override;

    void on_done(HandleDone handler) override;

private:
    struct Self;
    std::unique_ptr<Self> self;
};
}

#endif
