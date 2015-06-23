/*
 * Copyright © 2015 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_NESTED_MOCK_EGL_H_
#define MIR_TEST_DOUBLES_NESTED_MOCK_EGL_H_

#include "mir/test/doubles/mock_egl.h"

namespace mir
{
namespace test
{
namespace doubles
{
/// MockEGL with configuration for operating a nested server.    
class NestedMockEGL : public ::testing::NiceMock<MockEGL>
{
public:
    NestedMockEGL();

private:
    void egl_initialize(EGLint* major, EGLint* minor);
    void egl_choose_config(EGLConfig* config, EGLint*  num_config);
};
}
}
}

#endif /* MIR_TEST_DOUBLES_NESTED_MOCK_EGL_H_ */
