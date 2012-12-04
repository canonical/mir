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
#ifndef MIR_DRAW_PATTERNS_H
#define MIR_DRAW_PATTERNS_H

namespace mir
{
namespace draw
{

class DrawPattern
{
public:
    virtual ~DrawPattern();
    virtual void draw(MirGraphicsRegion* region) const = 0;
    virtual bool check(const MirGraphicsRegion* region) const = 0;

};

class DrawPatternSolid : public DrawPattern
{
public:
    void draw(MirGraphicsRegion* region) const;
    bool check(const MirGraphicsRegion* region) const;
};
 
}
}

#endif /*MIR_DRAW_PATTERNS_H */
