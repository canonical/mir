/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "mir/graphics/egl_sync_fence.h"

namespace mg = mir::graphics;

void mg::NullCommandSync::raise()
{
}

void mg::NullCommandSync::reset()
{
}

bool mg::NullCommandSync::clear_or_timeout_after(std::chrono::nanoseconds)
{
    return true;
}

mg::EGLSyncFence::EGLSyncFence(std::shared_ptr<EGLSyncExtensions> const& egl) :
    egl(egl)
{
}

void mg::EGLSyncFence::raise()
{
}

void mg::EGLSyncFence::reset()
{
}

bool mg::EGLSyncFence::clear_or_timeout_after(std::chrono::nanoseconds ns)
{
    (void) ns;
    return false;
}
