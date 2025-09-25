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

#ifndef MIR_TEST_DOUBLES_MOCK_COMPOSITOR_H_
#define MIR_TEST_DOUBLES_MOCK_COMPOSITOR_H_

#include "mir/compositor/compositor.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockCompositor : public compositor::Compositor
{
public:
    MOCK_METHOD(void, start, ());
    MOCK_METHOD(void, stop, ());
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_COMPOSITOR_H_ */
