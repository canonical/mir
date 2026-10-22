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

#ifndef MOCK_KMS_FRAMEBUFFER_H_
#define MOCK_KMS_FRAMEBUFFER_H_

#include <mir/graphics/kms_framebuffer.h>
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
class MockKMSFramebuffer : public graphics::FBHandle
{
public:
    ~MockKMSFramebuffer() override
    {
        destroyed();
    }

    operator uint32_t() const override
    {
        return operator_uint32_thunk();
    }

    MOCK_METHOD(uint32_t, operator_uint32_thunk, (), (const));
    MOCK_METHOD(geometry::Size, size, (), (const, override));
    /// Called from the destructor, so tests can pin when a framebuffer is released
    MOCK_METHOD(void, destroyed, ());
};
}
}

#endif // MOCK_KMS_FRAMEBUFFER_H_
