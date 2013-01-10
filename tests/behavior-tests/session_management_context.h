/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_TEST_CUCUMBER_SESSION_MANAGEMENT_CONTEXT_H_
#define MIR_TEST_CUCUMBER_SESSION_MANAGEMENT_CONTEXT_H_

#include "mir/geometry/rectangle.h"
// TODO: This is for SurfaceId only? Seems incorrect
#include "mir/sessions/session.h"

#include <string>
#include <map>
#include <memory>
#include <tuple>

namespace mir
{

namespace sessions
{
class SessionManager;
}

namespace test
{
namespace cucumber
{

class DummyViewableArea;

class SessionManagementContext
{
public:
    SessionManagementContext();
    virtual ~SessionManagementContext() {}
    
    bool open_window_consuming(std::string const& window_name);
    bool open_window_with_size(std::string const& window_name, geometry::Size const& size);

    geometry::Size get_window_size(std::string const& window_name);
    
    void set_view_area(geometry::Rectangle const& new_view_region);

protected:
    SessionManagementContext(const SessionManagementContext&) = delete;
    SessionManagementContext& operator=(const SessionManagementContext&) = delete;

private:
    std::map<std::string, std::tuple<std::shared_ptr<sessions::Session>, sessions::SurfaceId>> open_windows;
    std::shared_ptr<sessions::SessionManager> session_manager;
    std::shared_ptr<DummyViewableArea> view_area;
};

}
}
} // namespace mir

#endif // MIR_TEST_CUCUMBER_SESSION_MANAGEMENT_CONTEXT_H_
