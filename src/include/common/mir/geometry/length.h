/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_GEOMETRY_LENGTH_H_
#define MIR_GEOMETRY_LENGTH_H_

namespace mir
{
namespace geometry
{

/// Length represents a physical length in the real world. The number of pixels
/// this equates to can then be calculated based on a given DPI.
class Length
{
public:
    enum Units
    {
        micrometres = 1,
        millimetres = 1000,
        centimetres = 10000,
        inches = 25400
    };

    Length() : magnitude(0) {}
    Length(Length const&) = default;
    Length& operator=(Length const&) = default;
    Length(float mag, Units units) : magnitude(mag * units) {}

    float as(Units units) const
    {
        return static_cast<float>(magnitude) / units;
    }

    float as_pixels(float dpi) const
    {
        return as(inches) * dpi;
    }

    bool operator==(Length const& rhs) const
    {
        return magnitude == rhs.magnitude;
    }

    bool operator!=(Length const& rhs) const
    {
        return magnitude != rhs.magnitude;
    }

private:
    float magnitude;
};

inline Length operator"" _mm(long double mag)
{
    return Length(mag, Length::millimetres);
}

inline Length operator"" _mm(unsigned long long mag)
{
    return Length(mag, Length::millimetres);
}

inline Length operator"" _cm(long double mag)
{
    return Length(mag, Length::centimetres);
}

inline Length operator"" _cm(unsigned long long mag)
{
    return Length(mag, Length::centimetres);
}

inline Length operator"" _in(long double mag)
{
    return Length(mag, Length::inches);
}

inline Length operator"" _in(unsigned long long mag)
{
    return Length(mag, Length::inches);
}

} // namespace geometry
} // namespace mir

#endif // MIR_GEOMETRY_LENGTH_H_
