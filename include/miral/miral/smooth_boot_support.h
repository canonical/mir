/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_SMOOTH_BOOT_SUPPORT_H
#define MIRAL_SMOOTH_BOOT_SUPPORT_H

#include <memory>

namespace mir { class Server; }

namespace miral
{

/// Provides a smooth boot transition from the prior screen (such as the plymouth
/// splash or any other DRM-based display) to the compositor.
class SmoothBootSupport
{
public:
    void operator()(mir::Server& server) const;

    explicit SmoothBootSupport();
    ~SmoothBootSupport();
    SmoothBootSupport(SmoothBootSupport const&);
    auto operator=(SmoothBootSupport const&) -> SmoothBootSupport&;

private:
    struct Self;
    std::shared_ptr<Self> self;
};

}

#endif
