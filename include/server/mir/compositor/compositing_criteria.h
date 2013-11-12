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

#ifndef MIR_COMPOSITOR_COMPOSITING_CRITERIA_H_
#define MIR_COMPOSITOR_COMPOSITING_CRITERIA_H_

#include <glm/glm.hpp>

namespace mir
{
namespace geometry
{
struct Rectangle;
}
namespace compositor
{

class CompositingCriteria 
{
public:
    virtual float alpha() const = 0;
    virtual glm::mat4 const& transformation() const = 0;
    virtual bool should_be_rendered_in(geometry::Rectangle const& rect) const = 0;
    virtual bool shaped() const = 0;  // meaning the pixel format has alpha

    virtual ~CompositingCriteria() = default;

protected:
    CompositingCriteria() = default;
    CompositingCriteria(CompositingCriteria const&) = delete;
    CompositingCriteria& operator=(CompositingCriteria const&) = delete;
};

}
}

#endif /* MIR_COMPOSITOR_COMPOSITING_CRITERIA_H_ */
