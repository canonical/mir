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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_DISPLAY_CHANGER_H_
#define MIR_TEST_DOUBLES_MOCK_DISPLAY_CHANGER_H_

#include "mir/frontend/display_changer.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockDisplayChanger : public frontend::DisplayChanger
{
public:
    MOCK_METHOD0(active_configuration, std::shared_ptr<graphics::DisplayConfiguration>());
    MOCK_METHOD2(configure,
        void(std::shared_ptr<frontend::Session> const&, std::shared_ptr<graphics::DisplayConfiguration> const&));
};

}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_DISPLAY_CHANGER_H_ */
