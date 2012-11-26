/*
 * Copyright © 2012 Canonical Ltd.
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

#ifndef MIR_TEST_MOCK_SURFACE_ORGANISER_H_
#define MIR_TEST_MOCK_SURFACE_ORGANISER_H_

#include "mir/frontend/application_surface_organiser.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mir
{
namespace frontend
{

struct MockSurfaceOrganiser : public SurfaceOrganiser
{
    MOCK_METHOD1(create_surface, std::weak_ptr<surfaces::Surface>(const surfaces::SurfaceCreationParameters&));
    MOCK_METHOD1(destroy_surface, void(std::weak_ptr<surfaces::Surface> const&));
    MOCK_METHOD1(hide_surface, void(std::weak_ptr<surfaces::Surface> const&));
    MOCK_METHOD1(show_surface, void(std::weak_ptr<surfaces::Surface> const&));
};

}
}

#endif // MIR_TEST_MOCK_SURFACE_ORGANISER_H_
