/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */
#ifndef MIR_TEST_FRAMEWORK_STUB_INPUT_PLATFORM_H_
#define MIR_TEST_FRAMEWORK_STUB_INPUT_PLATFORM_H_

#include "mir/input/platform.h"

namespace mir_test_framework
{
class StubInputPlatform : public mir::input::Platform
{
public:
    void start(mir::input::InputEventHandlerRegister& /*loop*/,
               std::shared_ptr<mir::input::InputDeviceRegistry> const& /*input_device_registry*/) override
    {
    }
    void stop(mir::input::InputEventHandlerRegister& /*loop*/) override
    {
    }
};

}

#endif
