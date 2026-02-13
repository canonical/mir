/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

use cxx::ExternType;

#[repr(C)]
#[derive(Copy, Clone)]
pub struct KeyEventData {
    pub has_time: bool,
    pub time_microseconds: u64,
    pub action: i32,
    pub scancode: u32,
}

unsafe impl ExternType for KeyEventData {
    type Id = cxx::type_id!("mir::input::evdev_rs::KeyEventData");
    type Kind = cxx::kind::Trivial;
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct PointerEventDataRs {
    pub has_time: bool,
    pub time_microseconds: u64,
    pub action: i32,
    pub buttons: u32,
    pub has_position: bool,
    pub position_x: f32,
    pub position_y: f32,
    pub displacement_x: f32,
    pub displacement_y: f32,
    pub axis_source: i32,
    pub precise_x: f32,
    pub discrete_x: i32,
    pub value120_x: i32,
    pub scroll_stop_x: bool,
    pub precise_y: f32,
    pub discrete_y: i32,
    pub value120_y: i32,
    pub scroll_stop_y: bool,
}

unsafe impl ExternType for PointerEventDataRs {
    type Id = cxx::type_id!("mir::input::evdev_rs::PointerEventData");
    type Kind = cxx::kind::Trivial;
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct TouchEventContactRs {
    pub touch_id: i32,
    pub action: i32,
    pub tooltype: i32,
    pub position_x: f32,
    pub position_y: f32,
    pub pressure: f32,
    pub touch_major: f32,
    pub touch_minor: f32,
    pub orientation: f32,
}

unsafe impl ExternType for TouchEventContactRs {
    type Id = cxx::type_id!("mir::input::evdev_rs::TouchContactData");
    type Kind = cxx::kind::Trivial;
}

// TouchEventDataRs is no longer used - TouchEventData is defined in the bridge instead
