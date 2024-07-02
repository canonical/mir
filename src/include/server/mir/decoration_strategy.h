/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdint>
namespace mir
{
class DecorationStrategy
{
public:
    enum class DecorationsType : uint32_t
    {
        csd,
        ssd
    };

    virtual ~DecorationStrategy() = default;
    virtual auto default_style() const -> DecorationsType = 0;
    virtual auto request_style(DecorationsType type) const -> DecorationsType = 0;
};
}
