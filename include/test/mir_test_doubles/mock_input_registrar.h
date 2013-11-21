/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_REGISTRAR_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_REGISTRAR_H_

#include "mir/scene/input_registrar.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockInputRegistrar : public scene::InputRegistrar
{
    virtual ~MockInputRegistrar() noexcept(true) {}
    MOCK_METHOD3(input_channel_opened, void(std::shared_ptr<input::InputChannel> const&,
                                            std::shared_ptr<input::Surface> const&,
                                            input::InputReceptionMode mode));
    MOCK_METHOD1(input_channel_closed, void(std::shared_ptr<input::InputChannel> const&));
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_INPUT_REGISTRAR_H_
