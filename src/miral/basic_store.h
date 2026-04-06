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

#ifndef MIRAL_BASIC_STORE_H
#define MIRAL_BASIC_STORE_H

#include <miral/live_config.h>

#include <filesystem>

namespace miral::live_config
{
class BasicStore
{
public:
    BasicStore();
    ~BasicStore();

    using HandleAttribute = std::function<void(Key const& key, std::optional<std::string_view> value)>;
    using HandleArrayAttribute = std::function<void(Key const& key, std::optional<std::span<std::string const>> value)>;

    void add_scalar_attribute(Key const& key, std::string_view description, HandleAttribute handler);
    void add_array_attribute(Key const& key, std::string_view description, HandleArrayAttribute handler);
    void on_done(std::function<void()> handler);

    void foreach_scalar_attribute(std::function<void(Key const&, std::string_view)> const& callback) const;
    void foreach_array_attribute(std::function<void(Key const&, std::string_view)> const& callback) const;

    void update_key(Key const& key, std::string_view value, std::filesystem::path const& modification_path);
    void do_transaction(std::function<void()> transaction_body);

private:
    class Self;
    std::shared_ptr<Self> self;
};
}

#endif
