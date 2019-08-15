/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_FRONTEND_DISPLAY_CHANGER_H_
#define MIR_FRONTEND_DISPLAY_CHANGER_H_

#include <memory>
#include <future>

namespace mir
{
namespace graphics
{
class DisplayConfiguration;
}
namespace scene
{
class Session;
}
namespace frontend
{

class DisplayChanger
{
public:
    virtual ~DisplayChanger() = default;

    virtual std::shared_ptr<graphics::DisplayConfiguration> base_configuration() = 0;
    virtual void configure(std::shared_ptr<scene::Session> const&, std::shared_ptr<graphics::DisplayConfiguration> const&) = 0;
    virtual void remove_session_configuration(std::shared_ptr<scene::Session> const&) = 0;
    virtual void set_base_configuration(std::shared_ptr<graphics::DisplayConfiguration> const&) = 0;
    virtual void preview_base_configuration(
        std::weak_ptr<scene::Session> const& session,
        std::shared_ptr<graphics::DisplayConfiguration> const& new_configuration,
        std::chrono::seconds timeout) = 0;
    virtual void confirm_base_configuration(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<graphics::DisplayConfiguration> const& confirmed_configuration) = 0;
    virtual void cancel_base_configuration_preview(std::shared_ptr<scene::Session> const& session) = 0;

protected:
    DisplayChanger() = default;
    DisplayChanger(DisplayChanger const&) = delete;
    DisplayChanger& operator=(DisplayChanger const&) = delete;
};

}
}

#endif /* MIR_FRONTEND_DISPLAY_CHANGER_H_ */
