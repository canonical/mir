/*
 * Copyright © 2022 Canonical Ltd.
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
 */

#ifndef MIR_GEOMETRY_FORWARD_H_
#define MIR_GEOMETRY_FORWARD_H_

#include "dimensions_generic.h"

namespace mir
{
namespace geometry
{
namespace generic
{
template<template<typename> typename T>
struct Point;

template<template<typename> typename T>
struct Size;

template<template<typename> typename T>
struct Displacement;

template<typename P, typename S>
struct Rectangle;
}

using Point = generic::Point<generic::Value<int>::Wrapper>;
using Size = generic::Size<generic::Value<int>::Wrapper>;
using Displacement = generic::Displacement<generic::Value<int>::Wrapper>;
using Rectangle = generic::Rectangle<Point, Size>;
}
}

#endif // MIR_GEOMETRY_FORWARD_H_
