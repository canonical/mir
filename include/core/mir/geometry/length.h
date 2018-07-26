/*
 * Copyright © 2014, 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

    constexpr Length() : magnitude(0) {}
    constexpr Length(Length const&) = default;
    constexpr Length(float mag, Units units) : magnitude(mag * units) {}
    Length& operator=(Length const&) = default;

    constexpr float as(Units units) const
    {
        return static_cast<float>(magnitude) / units;
    }

    constexpr float as_pixels(float dpi) const
    {
        return as(inches) * dpi;
    }

    constexpr bool operator==(Length const& rhs) const
    {
        return magnitude == rhs.magnitude;
    }

    constexpr bool operator!=(Length const& rhs) const
    {
        return magnitude != rhs.magnitude;
    }

private:
    float magnitude;
};

inline constexpr Length operator"" _mm(long double mag)
{
    return Length(mag, Length::millimetres);
}

inline constexpr Length operator"" _mm(unsigned long long mag)
{
    return Length(mag, Length::millimetres);
}

inline constexpr Length operator"" _cm(long double mag)
{
    return Length(mag, Length::centimetres);
}

inline constexpr Length operator"" _cm(unsigned long long mag)
{
    return Length(mag, Length::centimetres);
}

inline constexpr Length operator"" _in(long double mag)
{
    return Length(mag, Length::inches);
}

inline constexpr Length operator"" _in(unsigned long long mag)
{
    return Length(mag, Length::inches);
}

} // namespace geometry
} // namespace mir

#endif // MIR_GEOMETRY_LENGTH_H_
