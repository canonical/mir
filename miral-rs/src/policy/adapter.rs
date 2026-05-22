//! Adapter that bridges miral-rs WindowManagementPolicy to miral-sys PolicyBridge.

use miral_sys::ffi;
use miral_sys::PolicyBridge;

use crate::application::ApplicationInfo;
use crate::geometry::{Displacement, Point, Rectangle, Size};
use crate::input::{
    KeyAction, KeyboardEvent, Modifiers, PointerAction, PointerEvent, TouchAction, TouchEvent,
};
use crate::output::{Output, Zone};
use crate::policy::{Advice, WindowManagementPolicy};
use crate::window::{WindowInfo, WindowSpecification, WindowState};

/// Adapts a user-defined `WindowManagementPolicy` into the FFI `PolicyBridge` trait.
pub(crate) struct PolicyBridgeAdapter<P: WindowManagementPolicy> {
    policy: P,
}

impl<P: WindowManagementPolicy> PolicyBridgeAdapter<P> {
    pub fn new(policy: P) -> Self {
        Self { policy }
    }

    fn make_window_info(info: &ffi::MiralWindowInfo) -> WindowInfo {
        let snapshot = ffi::miral_window_info_snapshot(info);
        let window_id = ffi::miral_window_info_id(info);
        WindowInfo::from_ffi(&snapshot, window_id)
    }

    fn make_app_info(info: &ffi::MiralApplicationInfo) -> ApplicationInfo {
        let snapshot = ffi::miral_app_info_snapshot(info);
        let app_id = ffi::miral_app_info_id(info);
        ApplicationInfo::from_ffi(app_id, snapshot.name.to_string())
    }
}

impl<P: WindowManagementPolicy> PolicyBridge for PolicyBridgeAdapter<P> {
    fn set_tools_ptr(&mut self, tools_ptr: u64) {
        let ptr = tools_ptr as *mut ffi::MiralTools;
        self.policy.tools_mut().set_raw(ptr);
    }

    fn place_new_window(
        &mut self,
        app_info: &ffi::MiralApplicationInfo,
        spec: &ffi::WindowSpecData,
    ) -> ffi::WindowSpecData {
        let app = Self::make_app_info(app_info);
        let requested = WindowSpecification::from_ffi(spec);
        let result = self.policy.place_new_window(&app, &requested);
        result.to_ffi()
    }

    fn handle_window_ready(&mut self, window_info: &ffi::MiralWindowInfo) {
        let info = Self::make_window_info(window_info);
        self.policy.handle_window_ready(&info);
    }

    fn handle_modify_window(
        &mut self,
        window_info: &ffi::MiralWindowInfo,
        spec: &ffi::WindowSpecData,
    ) {
        let info = Self::make_window_info(window_info);
        let modifications = WindowSpecification::from_ffi(spec);
        self.policy.handle_modify_window(&info, &modifications);
    }

    fn handle_raise_window(&mut self, window_info: &ffi::MiralWindowInfo) {
        let info = Self::make_window_info(window_info);
        self.policy.handle_raise_window(&info);
    }

    fn handle_keyboard_event(&mut self, event: &ffi::KeyboardEventInfo) -> bool {
        let kb_event = KeyboardEvent {
            action: KeyAction::from_raw(event.action),
            key_code: event.scan_code,
            keysym: event.keysym as u32,
            modifiers: Modifiers::from_raw(event.modifiers),
            timestamp_ns: event.event_time,
        };
        self.policy.handle_keyboard_event(&kb_event)
    }

    fn handle_touch_event(&mut self, event: &ffi::TouchEventInfo) -> bool {
        let touch_event = TouchEvent {
            action: TouchAction::from_raw(event.action),
            touch_id: event.id,
            x: event.x,
            y: event.y,
            modifiers: Modifiers::from_raw(event.modifiers),
            timestamp_ns: event.event_time,
        };
        self.policy.handle_touch_event(&touch_event)
    }

    fn handle_pointer_event(&mut self, event: &ffi::PointerEventInfo) -> bool {
        let ptr_event = PointerEvent {
            action: PointerAction::from_raw(event.action),
            x: event.x,
            y: event.y,
            hscroll: event.dx,
            vscroll: event.dy,
            modifiers: Modifiers::from_raw(event.modifiers),
            button: event.buttons,
            timestamp_ns: event.event_time,
        };
        self.policy.handle_pointer_event(&ptr_event)
    }

    fn handle_request_move(&mut self, window_info: &ffi::MiralWindowInfo) {
        let info = Self::make_window_info(window_info);
        let input_event = crate::input::InputEvent { timestamp_ns: 0 };
        self.policy.handle_request_move(&info, &input_event);
    }

    fn handle_request_resize(&mut self, window_info: &ffi::MiralWindowInfo, edge: i32) {
        let info = Self::make_window_info(window_info);
        let input_event = crate::input::InputEvent { timestamp_ns: 0 };
        self.policy
            .handle_request_resize(&info, &input_event, edge as u32);
    }

    fn confirm_inherited_move(
        &mut self,
        window_info: &ffi::MiralWindowInfo,
        movement: ffi::Displacement,
    ) -> ffi::Rectangle {
        let info = Self::make_window_info(window_info);
        let disp = Displacement::from(movement);
        let result = self.policy.confirm_inherited_move(&info, disp);
        result.into()
    }

    fn confirm_placement_on_display(
        &mut self,
        window_info: &ffi::MiralWindowInfo,
        new_state: i32,
        new_placement: &ffi::Rectangle,
    ) -> ffi::Rectangle {
        let info = Self::make_window_info(window_info);
        let state = WindowState::from_raw(new_state);
        let placement: Rectangle = (*new_placement).into();
        let result = self.policy.confirm_placement_on_display(&info, state, placement);
        result.into()
    }

    fn advise_new_app(&mut self, app_info: &ffi::MiralApplicationInfo) {
        let app = Self::make_app_info(app_info);
        self.policy.advise(Advice::NewApp { app });
    }

    fn advise_delete_app(&mut self, app_info: &ffi::MiralApplicationInfo) {
        let app = Self::make_app_info(app_info);
        self.policy.advise(Advice::DeleteApp { app });
    }

    fn advise_new_window(&mut self, window_info: &ffi::MiralWindowInfo) {
        let info = Self::make_window_info(window_info);
        self.policy.advise(Advice::NewWindow { window_info: info });
    }

    fn advise_delete_window(&mut self, window_info: &ffi::MiralWindowInfo) {
        let info = Self::make_window_info(window_info);
        self.policy
            .advise(Advice::DeleteWindow { window_info: info });
    }

    fn advise_focus_gained(&mut self, window_info: &ffi::MiralWindowInfo) {
        let info = Self::make_window_info(window_info);
        self.policy
            .advise(Advice::FocusGained { window_info: info });
    }

    fn advise_focus_lost(&mut self, window_info: &ffi::MiralWindowInfo) {
        let info = Self::make_window_info(window_info);
        self.policy.advise(Advice::FocusLost { window_info: info });
    }

    fn advise_state_change(&mut self, window_info: &ffi::MiralWindowInfo, state: i32) {
        let info = Self::make_window_info(window_info);
        let state = WindowState::from_raw(state);
        self.policy.advise(Advice::StateChange {
            window_info: info,
            state,
        });
    }

    fn advise_move_to(&mut self, window_info: &ffi::MiralWindowInfo, top_left: ffi::Point) {
        let info = Self::make_window_info(window_info);
        let point = Point::from(top_left);
        self.policy.advise(Advice::MoveTo {
            window_info: info,
            top_left: point,
        });
    }

    fn advise_resize(&mut self, window_info: &ffi::MiralWindowInfo, new_size: ffi::Size) {
        let info = Self::make_window_info(window_info);
        let size = Size::from(new_size);
        self.policy.advise(Advice::Resize {
            window_info: info,
            new_size: size,
        });
    }

    fn advise_begin(&mut self) {
        self.policy.advise(Advice::Begin);
    }

    fn advise_end(&mut self) {
        self.policy.advise(Advice::End);
    }

    fn advise_output_create(&mut self, output: &ffi::OutputSnapshot) {
        let out = Output::new(
            output.id as u32,
            output.extents.into(),
            output.name.to_string(),
        );
        self.policy.advise(Advice::OutputCreate { output: out });
    }

    fn advise_output_update(&mut self, updated: &ffi::OutputSnapshot, original: &ffi::OutputSnapshot) {
        let upd = Output::new(
            updated.id as u32,
            updated.extents.into(),
            updated.name.to_string(),
        );
        let orig = Output::new(
            original.id as u32,
            original.extents.into(),
            original.name.to_string(),
        );
        self.policy.advise(Advice::OutputUpdate {
            updated: upd,
            original: orig,
        });
    }

    fn advise_output_delete(&mut self, output: &ffi::OutputSnapshot) {
        let out = Output::new(
            output.id as u32,
            output.extents.into(),
            output.name.to_string(),
        );
        self.policy.advise(Advice::OutputDelete { output: out });
    }

    fn advise_zone_create(&mut self, zone: &ffi::ZoneSnapshot) {
        let z = Zone::new(zone.extents.into());
        self.policy.advise(Advice::ZoneCreate { zone: z });
    }

    fn advise_zone_update(&mut self, updated: &ffi::ZoneSnapshot, original: &ffi::ZoneSnapshot) {
        let upd = Zone::new(updated.extents.into());
        let orig = Zone::new(original.extents.into());
        self.policy.advise(Advice::ZoneUpdate {
            updated: upd,
            original: orig,
        });
    }

    fn advise_zone_delete(&mut self, zone: &ffi::ZoneSnapshot) {
        let z = Zone::new(zone.extents.into());
        self.policy.advise(Advice::ZoneDelete { zone: z });
    }
}
