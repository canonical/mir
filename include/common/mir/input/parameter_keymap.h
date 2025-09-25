/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_INPUT_PARAMETER_KEYMAP_H_
#define MIR_INPUT_PARAMETER_KEYMAP_H_

#include "keymap.h"

#include <string>
#include <memory>

namespace mir
{
namespace input
{

class ParameterKeymap
    : public Keymap
{
public:
    static auto constexpr default_model{"pc105+inet"};
    static auto constexpr default_layout{"us"};

    ParameterKeymap() = default;
    ParameterKeymap(std::string&& model, std::string&& layout, std::string&& variant, std::string&& options)
        : model_{model}, layout{layout}, variant{variant}, options{options}
    {
    }

    ParameterKeymap(
        std::string const& model,
        std::string const& layout,
        std::string const& variant,
        std::string const& options)
        : model_{model}, layout{layout}, variant{variant}, options{options}
    {
    }

    auto matches(Keymap const& other) const -> bool override;
    auto model() const -> std::string override;
    auto make_unique_xkb_keymap(xkb_context* context) const -> XKBKeymapPtr override;
    auto with_layout(std::string const& layout, std::string const& variant) const -> std::unique_ptr<Keymap>;

private:
    std::string const model_{default_model};
    std::string const layout{default_layout};
    std::string const variant;
    std::string const options;
};

}
}

#endif // MIR_INPUT_PARAMETER_KEYMAP_H_
