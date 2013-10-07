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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_COMPOSITING_CRITERIA_H_
#define MIR_TEST_DOUBLES_STUB_COMPOSITING_CRITERIA_H_

#include "mir/compositor/compositing_criteria.h"
#include <glm/gtc/matrix_transform.hpp>
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class StubCompositingCriteria : public compositor::CompositingCriteria
{
public:
    StubCompositingCriteria(int x, int y, int width, int height,
                            float opacity=1.0f,
                            bool rectangular=true)
        : rect{{x, y}, {width, height}},
          opacity(opacity),
          rectangular(rectangular)
    {
        const glm::mat4 ident;
        glm::vec3 size(width, height, 0.0f);
        glm::vec3 pos(x + width / 2, y + height / 2, 0.0f);
        trans = glm::scale( glm::translate(ident, pos), size);
    }

    float alpha() const override
    {
        return opacity;
    }

    glm::mat4 const& transformation() const override
    {
        return trans;
    }

    bool should_be_rendered_in(const mir::geometry::Rectangle &r) const override
    {
        return rect.overlaps(r);
    }

    bool shaped() const override
    {
        return !rectangular;
    }

private:
    mir::geometry::Rectangle rect;
    glm::mat4 trans;
    float opacity;
    bool rectangular;
};

} // namespace doubles
} // namespace test
} // namespace mir
#endif // MIR_TEST_DOUBLES_STUB_COMPOSITING_CRITERIA_H_
