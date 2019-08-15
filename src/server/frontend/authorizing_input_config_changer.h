/*
 * Copyright © 2017 Canonical Ltd.
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
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_FRONTEND_AUTHORIZING_INPUT_CONFIG_CHANGER_H_
#define MIR_FRONTEND_AUTHORIZING_INPUT_CONFIG_CHANGER_H_

#include "mir/frontend/input_configuration_changer.h"

namespace mir
{
namespace frontend
{
class AuthorizingInputConfigChanger : public frontend::InputConfigurationChanger
{
public:
    AuthorizingInputConfigChanger(std::shared_ptr<InputConfigurationChanger> const &changer,
                                  bool configure_input_is_allowed,
                                  bool set_base_configuration_is_allowed);

    MirInputConfig base_configuration() override;
    void configure(std::shared_ptr<scene::Session> const&, MirInputConfig &&) override;
    void set_base_configuration(MirInputConfig &&) override;
private:
    std::shared_ptr<InputConfigurationChanger> const changer;
    bool const configure_input_is_allowed;
    bool const set_base_configuration_is_allowed;
};

}
}

#endif
