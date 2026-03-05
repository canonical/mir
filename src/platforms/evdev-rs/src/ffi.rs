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
pub struct PointerEventData {
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

impl Default for PointerEventData {
    fn default() -> Self {
        Self {
            has_time: false,
            time_microseconds: 0,
            action: crate::MirPointerAction::mir_pointer_action_motion.repr,
            buttons: 0,
            has_position: false,
            position_x: 0.0,
            position_y: 0.0,
            displacement_x: 0.0,
            displacement_y: 0.0,
            axis_source: crate::MirPointerAxisSource::mir_pointer_axis_source_none.repr,
            precise_x: 0.0,
            discrete_x: 0,
            value120_x: 0,
            scroll_stop_x: false,
            precise_y: 0.0,
            discrete_y: 0,
            value120_y: 0,
            scroll_stop_y: false,
        }
    }
}

unsafe impl ExternType for PointerEventData {
    type Id = cxx::type_id!("mir::input::evdev_rs::PointerEventData");
    type Kind = cxx::kind::Trivial;
}
