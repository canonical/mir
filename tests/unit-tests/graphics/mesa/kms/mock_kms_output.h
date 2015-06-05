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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MOCK_KMS_OUTPUT_H_
#define MOCK_KMS_OUTPUT_H_

#include "src/platforms/mesa/server/kms/kms_output.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{

struct MockKMSOutput : public graphics::mesa::KMSOutput
{
    MOCK_METHOD0(reset, void());
    MOCK_METHOD2(configure, void(geometry::Displacement, size_t));
    MOCK_CONST_METHOD0(size, geometry::Size());
    MOCK_CONST_METHOD0(max_refresh_rate, int());

    MOCK_METHOD1(set_crtc, bool(uint32_t));
    MOCK_METHOD0(clear_crtc, void());
    MOCK_METHOD1(schedule_page_flip, bool(uint32_t));
    MOCK_METHOD0(wait_for_page_flip, void());

    MOCK_METHOD1(set_cursor, void(gbm_bo*));
    MOCK_METHOD1(move_cursor, void(geometry::Point));
    MOCK_METHOD0(clear_cursor, void());
    MOCK_CONST_METHOD0(has_cursor, bool());

    MOCK_METHOD1(set_power_mode, void(MirPowerMode));
};

} // namespace test
} // namespace mir

#endif // MOCK_KMS_OUTPUT_H_
