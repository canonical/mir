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

#include "mir/graphics/output_filter.h"

#include <memory>

namespace mir
{
namespace input { class Scene; }

namespace graphics
{

class DefaultOutputFilter : public OutputFilter
{
public:
    DefaultOutputFilter(
        std::shared_ptr<input::Scene> const& scene);

    std::string filter() override;
    void set_filter(std::string) override;

private:
    std::shared_ptr<input::Scene> const scene;
    std::string filter_;
};

}
}
