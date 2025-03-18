/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIRAL_CURSOR_SCALE_H
#define MIRAL_CURSOR_SCALE_H

#include <memory>

namespace mir { class Server; }

namespace miral
{
/// Allows for the configuration of the cursor's scale at runtime.
/// \remark Since MirAL 5.3
class CursorScale
{
public:
    explicit CursorScale();
    explicit CursorScale(float default_scale);
    ~CursorScale();

    /// Stores the value passed.
    /// \note Does not set the scale immediately. You need to call
    /// [apply_scale] for that
    void set_scale(float new_scale) const;

    /// Applies the set scale
    /// \note Cannot be called before the server is started.
    void apply_scale() const;

    void operator()(mir::Server& server) const;
private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif
