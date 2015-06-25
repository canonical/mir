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

#ifndef MIR_TEST_DOUBLES_NULL_DISPLAY_CHANGER_H_
#define MIR_TEST_DOUBLES_NULL_DISPLAY_CHANGER_H_

#include "mir/frontend/display_changer.h"
#include "mir/test/doubles/null_display_configuration.h"

namespace mir
{
namespace test
{
namespace doubles
{

class NullDisplayChanger : public frontend::DisplayChanger
{
public:
    virtual std::shared_ptr<graphics::DisplayConfiguration> active_configuration()
    {
        return std::make_shared<NullDisplayConfiguration>();
    }
    virtual void configure(std::shared_ptr<frontend::Session> const&, std::shared_ptr<graphics::DisplayConfiguration> const&)
    {
    }
};
}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_DISPLAY_CHANGER_H_ */
