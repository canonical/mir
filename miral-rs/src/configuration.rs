//! Configuration options for the Mir compositor.
//!
//! [`ConfigurationOption`] adds user-defined options to Mir's option handling.
//! Options can be supplied via command line, environment variables, config files,
//! or defaults. Callbacks fire on every value change, which may happen multiple
//! times throughout the program's lifetime.
//!
//! # Example
//!
//! ```rust,ignore
//! use miral::configuration::ConfigurationOption;
//! use miral::runner::MirRunner;
//!
//! MirRunner::new(std::env::args())
//!     .add_config_option(ConfigurationOption::new(
//!         "shell-terminal-emulator",
//!         "Terminal emulator to use",
//!         "xterm".to_string(),
//!         |cmd| { println!("Terminal set to: {cmd}"); },
//!     ))
//!     .add_config_option(ConfigurationOption::flag(
//!         "verbose",
//!         "Enable verbose logging",
//!         |is_set| { println!("Verbose: {is_set}"); },
//!     ).pre_init())
//!     .add_window_management_policy::<MyPolicy>()
//!     .run()
//!     .expect("Server failed");
//! ```

/// Types that can be used as configuration option values.
///
/// This is a sealed trait — only `i32`, `f64`, `String`, and `bool` implement it.
pub trait ConfigValue: private::Sealed {}

impl ConfigValue for i32 {}
impl ConfigValue for f64 {}
impl ConfigValue for String {}
impl ConfigValue for bool {}

/// Subset of [`ConfigValue`] that supports optional (no default) configuration.
///
/// C++ `mir::optional_value` exists for `int`, `string`, and `bool` — but not `double`.
pub trait OptionalConfigValue: ConfigValue + private::OptionalSealed {}

impl OptionalConfigValue for i32 {}
impl OptionalConfigValue for String {}
impl OptionalConfigValue for bool {}

mod private {
    pub trait Sealed {}
    impl Sealed for i32 {}
    impl Sealed for f64 {}
    impl Sealed for String {}
    impl Sealed for bool {}

    pub trait OptionalSealed {}
    impl OptionalSealed for i32 {}
    impl OptionalSealed for String {}
    impl OptionalSealed for bool {}
}

/// Option type tags matching the C++ bridge `ConfigOptionDesc::option_type`.
const OPTION_TYPE_INT: i32 = 0;
const OPTION_TYPE_DOUBLE: i32 = 1;
const OPTION_TYPE_STRING: i32 = 2;
const OPTION_TYPE_BOOL: i32 = 3;
const OPTION_TYPE_FLAG: i32 = 4;
const OPTION_TYPE_OPTIONAL_INT: i32 = 5;
const OPTION_TYPE_OPTIONAL_STRING: i32 = 6;
const OPTION_TYPE_OPTIONAL_BOOL: i32 = 7;
const OPTION_TYPE_MULTI: i32 = 8;

/// A user-defined configuration option for the Mir compositor.
///
/// Wraps `miral::ConfigurationOption`. Each option registers a name, description,
/// optional default value, and a callback that fires whenever the value changes.
///
/// Use the factory methods to create options for different value types:
/// - [`new`](ConfigurationOption::new) — required value with a default
/// - [`optional`](ConfigurationOption::optional) — optional value (no default)
/// - [`flag`](ConfigurationOption::flag) — presence-only flag
/// - [`multi`](ConfigurationOption::multi) — multi-value string list
///
/// Call [`pre_init`](ConfigurationOption::pre_init) to process the option before
/// Mir initialization starts.
pub struct ConfigurationOption {
    pub(crate) name: String,
    pub(crate) description: String,
    pub(crate) option_type: i32,
    pub(crate) default_int: i32,
    pub(crate) default_double: f64,
    pub(crate) default_string: String,
    pub(crate) default_bool: bool,
    pub(crate) callback: miral_sys::ConfigCallback,
    pub(crate) pre_init: bool,
}

/// Helper trait to route `ConfigurationOption::new` to the correct option type.
///
/// This is a sealed trait — users cannot implement it.
pub trait ConfigDefault: ConfigValue {
    /// Create a `ConfigurationOption` with the given default and callback.
    fn into_option(
        name: String,
        description: String,
        default: Self,
        callback: Box<dyn FnMut(Self) + Send>,
    ) -> ConfigurationOption;
}

impl ConfigDefault for i32 {
    fn into_option(
        name: String,
        description: String,
        default: Self,
        callback: Box<dyn FnMut(Self) + Send>,
    ) -> ConfigurationOption {
        ConfigurationOption {
            name,
            description,
            option_type: OPTION_TYPE_INT,
            default_int: default,
            default_double: 0.0,
            default_string: String::new(),
            default_bool: false,
            callback: miral_sys::ConfigCallback::Int(callback),
            pre_init: false,
        }
    }
}

impl ConfigDefault for f64 {
    fn into_option(
        name: String,
        description: String,
        default: Self,
        callback: Box<dyn FnMut(Self) + Send>,
    ) -> ConfigurationOption {
        ConfigurationOption {
            name,
            description,
            option_type: OPTION_TYPE_DOUBLE,
            default_int: 0,
            default_double: default,
            default_string: String::new(),
            default_bool: false,
            callback: miral_sys::ConfigCallback::Double(callback),
            pre_init: false,
        }
    }
}

impl ConfigDefault for String {
    fn into_option(
        name: String,
        description: String,
        default: Self,
        callback: Box<dyn FnMut(Self) + Send>,
    ) -> ConfigurationOption {
        ConfigurationOption {
            name,
            description,
            option_type: OPTION_TYPE_STRING,
            default_int: 0,
            default_double: 0.0,
            default_string: default,
            default_bool: false,
            callback: miral_sys::ConfigCallback::Str(callback),
            pre_init: false,
        }
    }
}

impl ConfigDefault for bool {
    fn into_option(
        name: String,
        description: String,
        default: Self,
        callback: Box<dyn FnMut(Self) + Send>,
    ) -> ConfigurationOption {
        ConfigurationOption {
            name,
            description,
            option_type: OPTION_TYPE_BOOL,
            default_int: 0,
            default_double: 0.0,
            default_string: String::new(),
            default_bool: default,
            callback: miral_sys::ConfigCallback::Bool(callback),
            pre_init: false,
        }
    }
}

/// Helper trait to route `ConfigurationOption::optional` to the correct option type.
///
/// This is a sealed trait — users cannot implement it.
pub trait ConfigOptional: OptionalConfigValue + Sized {
    /// Create an optional `ConfigurationOption` with the given callback.
    fn into_optional(
        name: String,
        description: String,
        callback: Box<dyn FnMut(Option<Self>) + Send>,
    ) -> ConfigurationOption;
}

impl ConfigOptional for i32 {
    fn into_optional(
        name: String,
        description: String,
        callback: Box<dyn FnMut(Option<Self>) + Send>,
    ) -> ConfigurationOption {
        ConfigurationOption {
            name,
            description,
            option_type: OPTION_TYPE_OPTIONAL_INT,
            default_int: 0,
            default_double: 0.0,
            default_string: String::new(),
            default_bool: false,
            callback: miral_sys::ConfigCallback::OptionalInt(callback),
            pre_init: false,
        }
    }
}

impl ConfigOptional for String {
    fn into_optional(
        name: String,
        description: String,
        callback: Box<dyn FnMut(Option<Self>) + Send>,
    ) -> ConfigurationOption {
        ConfigurationOption {
            name,
            description,
            option_type: OPTION_TYPE_OPTIONAL_STRING,
            default_int: 0,
            default_double: 0.0,
            default_string: String::new(),
            default_bool: false,
            callback: miral_sys::ConfigCallback::OptionalStr(callback),
            pre_init: false,
        }
    }
}

impl ConfigOptional for bool {
    fn into_optional(
        name: String,
        description: String,
        callback: Box<dyn FnMut(Option<Self>) + Send>,
    ) -> ConfigurationOption {
        ConfigurationOption {
            name,
            description,
            option_type: OPTION_TYPE_OPTIONAL_BOOL,
            default_int: 0,
            default_double: 0.0,
            default_string: String::new(),
            default_bool: false,
            callback: miral_sys::ConfigCallback::OptionalBool(callback),
            pre_init: false,
        }
    }
}

impl ConfigurationOption {
    /// Create a configuration option with a required value and default.
    ///
    /// The callback fires on every value change with the new value.
    /// Supported types: `i32`, `f64`, `String`, `bool`.
    ///
    /// # Example
    ///
    /// ```rust,ignore
    /// ConfigurationOption::new("port", "Server port", 8080, |port| {
    ///     println!("Port changed to {port}");
    /// })
    /// ```
    pub fn new<T: ConfigDefault>(
        name: impl Into<String>,
        description: impl Into<String>,
        default: T,
        callback: impl FnMut(T) + Send + 'static,
    ) -> Self {
        T::into_option(name.into(), description.into(), default, Box::new(callback))
    }

    /// Create a configuration option with an optional value (no default).
    ///
    /// The callback fires with `None` if the option is not set, or `Some(value)` if it is.
    /// Supported types: `i32`, `String`, `bool`.
    ///
    /// # Example
    ///
    /// ```rust,ignore
    /// ConfigurationOption::optional::<String>("log-file", "Log file path", |path| {
    ///     if let Some(path) = path {
    ///         println!("Logging to {path}");
    ///     }
    /// })
    /// ```
    pub fn optional<T: ConfigOptional>(
        name: impl Into<String>,
        description: impl Into<String>,
        callback: impl FnMut(Option<T>) + Send + 'static,
    ) -> Self {
        T::into_optional(name.into(), description.into(), Box::new(callback))
    }

    /// Create a presence-only flag option.
    ///
    /// The callback fires with `true` if the flag is present, `false` otherwise.
    ///
    /// # Example
    ///
    /// ```rust,ignore
    /// ConfigurationOption::flag("verbose", "Enable verbose logging", |is_set| {
    ///     println!("Verbose: {is_set}");
    /// })
    /// ```
    pub fn flag(
        name: impl Into<String>,
        description: impl Into<String>,
        callback: impl FnMut(bool) + Send + 'static,
    ) -> Self {
        Self {
            name: name.into(),
            description: description.into(),
            option_type: OPTION_TYPE_FLAG,
            default_int: 0,
            default_double: 0.0,
            default_string: String::new(),
            default_bool: false,
            callback: miral_sys::ConfigCallback::Flag(Box::new(callback)),
            pre_init: false,
        }
    }

    /// Create a multi-value string list option.
    ///
    /// The callback fires with the list of values.
    ///
    /// # Example
    ///
    /// ```rust,ignore
    /// ConfigurationOption::multi("startup-apps", "Apps to launch on startup", |apps| {
    ///     for app in apps {
    ///         println!("Launching: {app}");
    ///     }
    /// })
    /// ```
    pub fn multi(
        name: impl Into<String>,
        description: impl Into<String>,
        callback: impl FnMut(&[String]) + Send + 'static,
    ) -> Self {
        // Wrap the &[String] callback into a Vec<String> callback for the FFI layer
        let mut callback = callback;
        Self {
            name: name.into(),
            description: description.into(),
            option_type: OPTION_TYPE_MULTI,
            default_int: 0,
            default_double: 0.0,
            default_string: String::new(),
            default_bool: false,
            callback: miral_sys::ConfigCallback::Multi(Box::new(move |values: Vec<String>| {
                callback(&values);
            })),
            pre_init: false,
        }
    }

    /// Process this option before Mir initialization starts.
    ///
    /// By default, options are processed after Mir initialization but before
    /// the server starts. Call this to process the option earlier.
    pub fn pre_init(mut self) -> Self {
        self.pre_init = true;
        self
    }
}
