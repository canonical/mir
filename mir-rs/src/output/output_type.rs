//! Display output type.

use crate::geometry::Rectangle;
use crate::output::{FormFactor, Orientation, OutputType, PhysicalSizeMM, PowerMode};

/// Represents a physical display output.
///
/// Outputs correspond to monitors/screens connected to the system.
/// Each output has a unique ID, physical dimensions, and a logical
/// rectangle describing its position in the compositor's coordinate space.
#[derive(Debug, Clone)]
pub struct Output {
    id: i32,
    name: String,
    connected: bool,
    used: bool,
    extents: Rectangle,
    refresh_rate: f64,
    scale: f32,
    power_mode: PowerMode,
    orientation: Orientation,
    form_factor: FormFactor,
    output_type: OutputType,
    physical_size_mm: PhysicalSizeMM,
}

impl Output {
    /// Create an output from an FFI snapshot.
    pub(crate) fn from_ffi(snapshot: &mir_sys::ffi::OutputSnapshot) -> Self {
        Self {
            id: snapshot.id,
            name: snapshot.name.clone(),
            connected: snapshot.connected,
            used: snapshot.used,
            extents: snapshot.extents.into(),
            refresh_rate: snapshot.refresh_rate,
            scale: snapshot.scale,
            power_mode: PowerMode::from_raw(snapshot.power_mode),
            orientation: Orientation::from_raw(snapshot.orientation),
            form_factor: FormFactor::from_raw(snapshot.form_factor),
            output_type: OutputType::from_raw(snapshot.output_type),
            physical_size_mm: PhysicalSizeMM {
                width: snapshot.physical_width_mm,
                height: snapshot.physical_height_mm,
            },
        }
    }

    /// The unique identifier of this output.
    pub fn id(&self) -> i32 {
        self.id
    }

    /// The name of this output (e.g., "HDMI-1", "eDP-1").
    pub fn name(&self) -> &str {
        &self.name
    }

    /// Whether this output is physically connected.
    pub fn connected(&self) -> bool {
        self.connected
    }

    /// Whether this output is being used by the compositor.
    pub fn used(&self) -> bool {
        self.used
    }

    /// The logical extents of this output in the compositor's coordinate space.
    pub fn extents(&self) -> Rectangle {
        self.extents
    }

    /// The refresh rate in Hz.
    pub fn refresh_rate(&self) -> f64 {
        self.refresh_rate
    }

    /// The output scale factor.
    pub fn scale(&self) -> f32 {
        self.scale
    }

    /// The power mode of this output.
    pub fn power_mode(&self) -> PowerMode {
        self.power_mode
    }

    /// The orientation of this output.
    pub fn orientation(&self) -> Orientation {
        self.orientation
    }

    /// The form factor of this output.
    pub fn form_factor(&self) -> FormFactor {
        self.form_factor
    }

    /// The connector type of this output.
    pub fn output_type(&self) -> OutputType {
        self.output_type
    }

    /// The physical size of this output in millimeters.
    pub fn physical_size_mm(&self) -> PhysicalSizeMM {
        self.physical_size_mm
    }
}
