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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/draw/patterns.h"

namespace md=mir::draw;

md::DrawPatternSolid::DrawPatternSolid(uint32_t color_value)
 : color_value(color_value)
{
}

void md::DrawPatternSolid::draw(MirGraphicsRegion* /*region*/) const
{
}

bool md::DrawPatternSolid::check(const MirGraphicsRegion* /*region*/) const
{
    return false;
}
