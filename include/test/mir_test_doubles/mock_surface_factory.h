/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_SURFACE_FACTORY_H_
#define MIR_TEST_DOUBLES_MOCK_SURFACE_FACTORY_H_

#include "mir/shell/surface_factory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSurfaceFactory : public shell::SurfaceFactory
{
    MOCK_METHOD3(create_surface, std::shared_ptr<shell::Surface>(
        const shell::SurfaceCreationParameters&,
        frontend::SurfaceId,
        std::shared_ptr<frontend::EventSink> const&));
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_SURFACE_FACTORY_H_
