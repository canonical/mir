//! The compositor runner and builder.
//!
//! [`MirRunner`] is the entry point for starting a Mir compositor. It uses
//! a builder pattern to configure the server before starting it.

use crate::configuration::ConfigurationOption;
use crate::extensions::ServerExtension;
use crate::keymap::Keymap;
use crate::policy::adapter::PolicyBridgeAdapter;
use crate::policy::WindowManagementPolicy;

/// The main entry point for running a Mir compositor.
///
/// Use the builder methods to configure the server, then call [`run()`](MirRunner::run)
/// to start it. The `run()` method blocks until the server is shut down.
///
/// # Example
///
/// ```rust,ignore
/// use miral::prelude::*;
///
/// fn main() {
///     MirRunner::new(std::env::args())
///         .add(Decorations::prefer_csd())
///         .add(Keymap::new("us"))
///         .add_window_management_policy::<MyPolicy>()
///         .run()
///         .expect("Server failed");
/// }
/// ```
pub struct MirRunner {
    args: Vec<String>,
    extensions: Vec<Box<dyn ServerExtension>>,
    config_options: Vec<ConfigurationOption>,
    policy_factory: Option<Box<dyn FnOnce() -> Box<dyn miral_sys::PolicyBridge> + Send>>,
    on_start: Option<Box<dyn FnOnce() + Send>>,
    on_stop: Option<Box<dyn FnOnce() + Send>>,
}

impl MirRunner {
    /// Create a new MirRunner with the given command-line arguments.
    ///
    /// The arguments are passed to the underlying Mir server for processing.
    /// Standard Mir options (like `--display-config`) are handled automatically.
    pub fn new(args: impl IntoIterator<Item = String>) -> Self {
        Self {
            args: args.into_iter().collect(),
            extensions: Vec::new(),
            config_options: Vec::new(),
            policy_factory: None,
            on_start: None,
            on_stop: None,
        }
    }

    /// Add a server extension (e.g., [`WaylandExtensions`](crate::extensions::WaylandExtensions),
    /// [`Decorations`](crate::decorations::Decorations), [`Keymap`](crate::keymap::Keymap)).
    pub fn add(mut self, extension: impl ServerExtension) -> Self {
        self.extensions.push(Box::new(extension));
        self
    }

    /// Add a configuration option.
    ///
    /// Configuration options are processed during server initialization (or before,
    /// if [`pre_init`](ConfigurationOption::pre_init) was called). The callback fires
    /// on every value change.
    ///
    /// # Example
    ///
    /// ```rust,ignore
    /// use miral::configuration::ConfigurationOption;
    ///
    /// runner.add_config_option(ConfigurationOption::new(
    ///     "terminal-emulator",
    ///     "Terminal emulator to use",
    ///     "xterm".to_string(),
    ///     |cmd| { println!("Terminal: {cmd}"); },
    /// ))
    /// ```
    pub fn add_config_option(mut self, option: ConfigurationOption) -> Self {
        self.config_options.push(option);
        self
    }

    /// Set the window management policy type.
    ///
    /// The policy will be constructed with uninitialized [`WindowManagerTools`](crate::policy::WindowManagerTools)
    /// which are automatically initialized by the framework before any policy
    /// methods are called.
    ///
    /// The type must implement [`WindowManagementPolicy`] and [`Default`].
    pub fn add_window_management_policy<P>(mut self) -> Self
    where
        P: WindowManagementPolicy + Default,
    {
        self.policy_factory = Some(Box::new(|| {
            let policy = P::default();
            Box::new(PolicyBridgeAdapter::new(policy))
        }));
        self
    }

    /// Set a callback to run when the server has started.
    pub fn on_start(mut self, f: impl FnOnce() + Send + 'static) -> Self {
        self.on_start = Some(Box::new(f));
        self
    }

    /// Set a callback to run when the server is stopping.
    pub fn on_stop(mut self, f: impl FnOnce() + Send + 'static) -> Self {
        self.on_stop = Some(Box::new(f));
        self
    }

    /// Run the compositor, blocking until shutdown.
    ///
    /// Returns `Ok(())` on clean shutdown, or an error if the server
    /// failed to start or encountered a fatal error.
    pub fn run(self) -> Result<(), Box<dyn std::error::Error>> {
        let policy_factory = self.policy_factory.ok_or(
            "No window management policy set. Call add_window_management_policy() before run().",
        )?;

        // Register the policy factory in the thread-local for C++ to call
        miral_sys::set_policy_factory(Box::new(policy_factory));

        // Extract extension configuration
        let mut decoration_mode: i32 = 0;
        let mut keymap_layout = String::new();
        let mut x11_enabled = false;
        let mut external_launcher_enabled = false;

        for ext in &self.extensions {
            let name = ext.name();
            match name {
                "Decorations(PreferCSD)" => decoration_mode = 1,
                "Decorations(PreferSSD)" => decoration_mode = 2,
                "Decorations(AlwaysSSD)" => decoration_mode = 3,
                "Decorations(AlwaysCSD)" => decoration_mode = 4,
                "Keymap" => {
                    // Downcast to get the layout
                    // We use Any-based downcasting via the extension name + stored config
                }
                "X11Support" => x11_enabled = true,
                "ExternalClientLauncher" => external_launcher_enabled = true,
                _ => {} // WaylandExtensions and others handled by defaults
            }
        }

        // For Keymap, we need the layout string. Use as_any() downcasting.
        for ext in &self.extensions {
            if let Some(keymap) = ext.as_any().downcast_ref::<Keymap>() {
                keymap_layout = if let Some(variant) = keymap.variant() {
                    format!("{}({})", keymap.layout(), variant)
                } else {
                    keymap.layout().to_string()
                };
            }
        }

        // Create the C++ MirRunner
        let mut runner = miral_sys::ffi::miral_runner_new(&self.args);

        if external_launcher_enabled {
            miral_sys::ffi::miral_runner_enable_external_launcher(runner.pin_mut());
        }

        // Register lifecycle callbacks
        if let Some(on_start) = self.on_start {
            miral_sys::set_on_start_callback(Box::new(on_start));
            miral_sys::ffi::miral_runner_register_start_callback(runner.pin_mut());
        }
        if let Some(on_stop) = self.on_stop {
            miral_sys::set_on_stop_callback(Box::new(on_stop));
            miral_sys::ffi::miral_runner_register_stop_callback(runner.pin_mut());
        }

        // Register configuration option callbacks and build descriptors
        let mut config_descs = Vec::new();
        for option in self.config_options {
            let callback_id = miral_sys::register_config_callback(option.callback);
            config_descs.push(miral_sys::ffi::ConfigOptionDesc {
                name: option.name,
                description: option.description,
                option_type: option.option_type,
                default_int: option.default_int,
                default_double: option.default_double,
                default_string: option.default_string,
                default_bool: option.default_bool,
                callback_id,
                pre_init: option.pre_init,
            });
        }

        // Run with full configuration
        let result = miral_sys::ffi::miral_runner_run_with_config(
            runner.pin_mut(),
            decoration_mode,
            &keymap_layout,
            x11_enabled,
            &config_descs,
        );

        // Clean up config callbacks after run completes
        miral_sys::clear_config_callbacks();

        if result != 0 {
            Err(format!("Server exited with code {}", result).into())
        } else {
            Ok(())
        }
    }
}
