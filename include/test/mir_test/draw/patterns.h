/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_TEST_DRAW_PATTERNS_H
#define MIR_TEST_DRAW_PATTERNS_H

#include "mir_toolkit/mir_client_library.h"

#include <memory>
#include <stdexcept>
/* todo: replace with color value types */
#include <stdint.h>
#include <cstring>

namespace mir
{
namespace test
{
namespace draw
{

class DrawPattern
{
public:
    virtual ~DrawPattern() {};
    virtual void draw(MirGraphicsRegion const& region) const = 0;
    virtual bool check(MirGraphicsRegion const& region) const = 0;

protected:
    DrawPattern() = default;
    DrawPattern(DrawPattern const&) = delete;
    DrawPattern& operator=(DrawPattern const&) = delete;
};

class DrawPatternSolid : public DrawPattern
{
public:
    /* todo: should construct with a color value type, not an uint32 */
    DrawPatternSolid(uint32_t color_value);

    void draw(MirGraphicsRegion const& region) const;
    bool check(MirGraphicsRegion const& region) const;

private:
    const uint32_t color_value;
};

template<size_t Rows, size_t Cols>
class DrawPatternCheckered : public DrawPattern
{
public:
    /* todo: should construct with a color value type, not an uint32 */
    DrawPatternCheckered(uint32_t (&pattern) [Rows][Cols]);

    void draw(MirGraphicsRegion const& region) const;
    bool check(MirGraphicsRegion const& region) const;

private:
    uint32_t color_pattern [Rows][Cols];
};
#include "mir_test/draw/draw_pattern_checkered-inl.h"

}
}
}

#endif /*MIR_TEST_DRAW_PATTERNS_H */
