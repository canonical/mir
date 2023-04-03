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

#ifndef MIR_TEST_RENDERING_PLATFORM_H
#define MIR_TEST_RENDERING_PLATFORM_H

#include <gtest/gtest.h>

#include "platform_test_harness.h"

namespace mir
{
class SharedLibrary;
}

class RenderingPlatformTest : public testing::TestWithParam<mir::test::PlatformTestHarness*>
{
public:
    RenderingPlatformTest();
    virtual ~RenderingPlatformTest() override;

protected:
    std::shared_ptr<mir::SharedLibrary> platform_module;
};

#endif //MIR_TEST_RENDERING_PLATFORM_H
