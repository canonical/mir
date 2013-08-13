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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_DISPLAY_CHANGER_H_
#define MIR_DISPLAY_CHANGER_H_

#include <memory>

namespace mir
{
namespace graphics
{
class DisplayConfiguration;
}

class DisplayChanger
{
public:
    virtual ~DisplayChanger() = default;

    enum SystemStateHandling : bool { RetainSystemState, PauseResumeSystem };

    virtual void configure_for_hardware_change(
        std::shared_ptr<graphics::DisplayConfiguration> const& conf,
        SystemStateHandling pause_resume_system) = 0;

protected:
    DisplayChanger() = default;
    DisplayChanger(DisplayChanger const&) = delete;
    DisplayChanger& operator=(DisplayChanger const&) = delete;
};

}

#endif /* MIR_DISPLAY_CHANGER_H_ */
