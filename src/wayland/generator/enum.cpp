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

#include "enum.h"
#include "utils.h"

#include <libxml++/libxml++.h>

Enum::Enum(xmlpp::Element const& node, std::string const& class_name)
    : name{sanitize_name(to_camel_case(node.get_attribute_value("name")))},
      class_name{class_name}
{
    for (auto const& child : node.get_children("entry"))
    {
        auto entry_node = dynamic_cast<xmlpp::Element const*>(child);
        std::string entry_name = sanitize_name(entry_node->get_attribute_value("name"));
        std::string entry_value_str = entry_node->get_attribute_value("value");
        entries.emplace_back(Entry{entry_name, entry_value_str});
    }
}

Emitter Enum::declaration() const
{
    std::vector<Emitter> body;
    for (auto const& entry : entries)
    {
        body.push_back({"static uint32_t const ", entry.name, " = ", entry.value, ";"});
    }

    return Lines{
        {"struct ", name},
        {Block{
            body
        }, ";"}
    };
}

Emitter Enum::impl() const
{
    std::vector<Emitter> lines;
    for (auto const& entry : entries)
    {
        lines.push_back({"uint32_t const mw::", class_name, "::", name, "::", entry.name, ";"});
    }

    return Lines{lines};
}
