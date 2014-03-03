/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "testdraw/graphics_region_factory.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mtd=mir::test::draw;

namespace
{

class MesaGraphicsRegionFactory : public mir::test::draw::GraphicsRegionFactory
{
public:
    std::shared_ptr<MirGraphicsRegion> graphic_region_from_handle(mir::graphics::NativeBuffer const&)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("cannot map graphic region yet"));
    }
};

}

std::shared_ptr<mtd::GraphicsRegionFactory> mtd::create_graphics_region_factory()
{
    return std::make_shared<MesaGraphicsRegionFactory>();
}
