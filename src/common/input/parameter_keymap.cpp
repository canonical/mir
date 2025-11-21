/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/input/parameter_keymap.h>

#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <xkbcommon/xkbcommon.h>

namespace mi = mir::input;

auto mi::ParameterKeymap::matches(Keymap const& other) const -> bool
{
    auto const params = dynamic_cast<ParameterKeymap const*>(&other);
    return params &&
        model_ == params->model_ &&
        layout == params->layout &&
        variant == params->variant &&
        options == params->options;
}

auto mi::ParameterKeymap::model() const -> std::string
{
    return model_;
}

auto mi::ParameterKeymap::make_unique_xkb_keymap(xkb_context* context) const -> XKBKeymapPtr
{
    xkb_rule_names keymap_names
    {
        "evdev",
        model_.c_str(),
        layout.c_str(),
        variant.c_str(),
        options.c_str()
    };
    auto keymap_ptr = xkb_keymap_new_from_names(context, &keymap_names, xkb_keymap_compile_flags(0));

    if (!keymap_ptr)
    {
        auto const error =
            "Illegal keymap configuration evdev-" +
            model_ + "-" + layout + "-" + variant + "-" + options;
        BOOST_THROW_EXCEPTION(std::invalid_argument(error.c_str()));
    }
    return {keymap_ptr, &xkb_keymap_unref};
}

auto mi::ParameterKeymap::with_layout(std::string const& layout, std::string const& variant) const
-> std::unique_ptr<Keymap>
{
    return std::make_unique<ParameterKeymap>(model_, layout, variant, options);
}
