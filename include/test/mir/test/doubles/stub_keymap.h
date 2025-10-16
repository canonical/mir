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

#ifndef MIR_TEST_DOUBLES_STUB_KEYMAP_H_
#define MIR_TEST_DOUBLES_STUB_KEYMAP_H_

#include <mir/input/keymap.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct StubKeymap : public mir::input::Keymap
{
    auto matches(Keymap const& other) const -> bool override { return &other == this; }
    auto model() const -> std::string override { return "stub_keymap_model"; }
    auto make_unique_xkb_keymap(xkb_context*) const -> mir::input::XKBKeymapPtr override
    {
        return {nullptr, [](auto){}};
    }
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_KEYMAP_H_
