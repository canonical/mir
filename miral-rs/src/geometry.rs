/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//! Geometry primitives for window and output positioning.
//!
//! These types represent positions, sizes, and regions in the compositor's
//! coordinate space. All values are in logical pixels.

use std::ops::{Add, Sub};

/// A 2D point in logical pixel coordinates.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub struct Point {
    /// The x coordinate.
    pub x: i32,
    /// The y coordinate.
    pub y: i32,
}

impl Point {
    /// Create a new point.
    pub fn new(x: i32, y: i32) -> Self {
        Self { x, y }
    }
}

impl Add<Displacement> for Point {
    type Output = Point;
    fn add(self, rhs: Displacement) -> Point {
        Point {
            x: self.x + rhs.dx,
            y: self.y + rhs.dy,
        }
    }
}

impl Sub<Point> for Point {
    type Output = Displacement;
    fn sub(self, rhs: Point) -> Displacement {
        Displacement {
            dx: self.x - rhs.x,
            dy: self.y - rhs.y,
        }
    }
}

/// A 2D size in logical pixels.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub struct Size {
    /// The width in logical pixels.
    pub width: i32,
    /// The height in logical pixels.
    pub height: i32,
}

impl Size {
    /// Create a new size.
    pub fn new(width: i32, height: i32) -> Self {
        Self { width, height }
    }
}

/// A rectangle defined by its top-left corner and size.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub struct Rectangle {
    /// The top-left corner of the rectangle.
    pub top_left: Point,
    /// The size of the rectangle.
    pub size: Size,
}

impl Rectangle {
    /// Create a new rectangle.
    pub fn new(top_left: Point, size: Size) -> Self {
        Self { top_left, size }
    }

    /// Check if a point is contained within this rectangle.
    pub fn contains(&self, point: Point) -> bool {
        point.x >= self.top_left.x
            && point.x < self.top_left.x + self.size.width
            && point.y >= self.top_left.y
            && point.y < self.top_left.y + self.size.height
    }
}

/// A 2D displacement vector in logical pixels.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub struct Displacement {
    /// The horizontal displacement.
    pub dx: i32,
    /// The vertical displacement.
    pub dy: i32,
}

impl Displacement {
    /// Create a new displacement.
    pub fn new(dx: i32, dy: i32) -> Self {
        Self { dx, dy }
    }
}

impl Add for Displacement {
    type Output = Displacement;
    fn add(self, rhs: Displacement) -> Displacement {
        Displacement {
            dx: self.dx + rhs.dx,
            dy: self.dy + rhs.dy,
        }
    }
}

impl Sub for Displacement {
    type Output = Displacement;
    fn sub(self, rhs: Displacement) -> Displacement {
        Displacement {
            dx: self.dx - rhs.dx,
            dy: self.dy - rhs.dy,
        }
    }
}

/// A 2D point with floating-point coordinates (for sub-pixel precision).
///
/// Used for cursor positioning where sub-pixel accuracy is needed.
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct PointF {
    /// The x coordinate.
    pub x: f32,
    /// The y coordinate.
    pub y: f32,
}

impl PointF {
    /// Create a new floating-point point.
    pub fn new(x: f32, y: f32) -> Self {
        Self { x, y }
    }
}

// --- FFI conversions ---

impl From<miral_sys::ffi::Point> for Point {
    fn from(p: miral_sys::ffi::Point) -> Self {
        Self { x: p.x, y: p.y }
    }
}

impl From<Point> for miral_sys::ffi::Point {
    fn from(p: Point) -> Self {
        Self { x: p.x, y: p.y }
    }
}

impl From<miral_sys::ffi::Size> for Size {
    fn from(s: miral_sys::ffi::Size) -> Self {
        Self {
            width: s.width,
            height: s.height,
        }
    }
}

impl From<Size> for miral_sys::ffi::Size {
    fn from(s: Size) -> Self {
        Self {
            width: s.width,
            height: s.height,
        }
    }
}

impl From<miral_sys::ffi::Rectangle> for Rectangle {
    fn from(r: miral_sys::ffi::Rectangle) -> Self {
        Self {
            top_left: r.top_left.into(),
            size: r.size.into(),
        }
    }
}

impl From<Rectangle> for miral_sys::ffi::Rectangle {
    fn from(r: Rectangle) -> Self {
        Self {
            top_left: r.top_left.into(),
            size: r.size.into(),
        }
    }
}

impl From<miral_sys::ffi::Displacement> for Displacement {
    fn from(d: miral_sys::ffi::Displacement) -> Self {
        Self { dx: d.dx, dy: d.dy }
    }
}

impl From<Displacement> for miral_sys::ffi::Displacement {
    fn from(d: Displacement) -> Self {
        Self { dx: d.dx, dy: d.dy }
    }
}
