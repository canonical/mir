/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MOCK_KMS_OUTPUT_H_
#define MOCK_KMS_OUTPUT_H_

#include "src/platforms/atomic-kms/server/kms/kms_output.h"
#include <gmock/gmock.h>

namespace mir
{

namespace test
{

class MockKMSOutput : public graphics::atomic::KMSOutput
{
public:
    MOCK_METHOD(uint32_t, id, (), (const, override));
    MOCK_METHOD(void, reset, (), (override));
    MOCK_METHOD(void, configure, (geometry::Displacement, size_t), (override));
    MOCK_METHOD(geometry::Size, size, (), (const, override));
    MOCK_METHOD(int, max_refresh_rate, (), (const, override));
    MOCK_METHOD(bool, set_crtc, (graphics::FBHandle const&), (override));
    MOCK_METHOD(bool, has_crtc_mismatch, (), (override));
    MOCK_METHOD(void, clear_crtc, (), (override));
    MOCK_METHOD(bool, page_flip, (graphics::FBHandle const&), (override));
    MOCK_METHOD(void, set_cursor_image, (gbm_bo*), (override));
    MOCK_METHOD(void, move_cursor, (geometry::Point), (override));
    MOCK_METHOD(bool, clear_cursor, (), (override));
    MOCK_METHOD(bool, has_cursor_image, (), (const, override));
    MOCK_METHOD(void, set_power_mode, (MirPowerMode), (override));
    MOCK_METHOD(void, set_gamma, (graphics::GammaCurves const&), (override));
    MOCK_METHOD(void, refresh_hardware_state, (), (override));
    MOCK_METHOD(void, update_from_hardware_state, (graphics::DisplayConfigurationOutput&), (const, override));
    MOCK_METHOD(int, drm_fd, (), (const, override));
};

} // namespace test
} // namespace mir

#endif // MOCK_KMS_OUTPUT_H_
