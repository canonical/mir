//! The compositor runner and builder.
//!
//! [`MirRunner`] is the entry point for starting a Mir compositor. It uses
//! a builder pattern to configure the server before starting it.

use std::sync::Arc;

use crate::configuration::ConfigurationOption;
use crate::extensions::ServerExtension;
use crate::idle::IdleListener;
use crate::keymap::Keymap;
use crate::magnifier::Magnifier;
use crate::policy::adapter::PolicyBridgeAdapter;
use crate::policy::WindowManagementPolicy;
use crate::session_lock::SessionLockListener;

/// A handle to a running compositor that can be used to request shutdown.
///
/// Obtain this via [`MirRunner::on_start`] and store it for later use.
/// The handle is `Send + Sync` and can be safely shared across threads.
///
/// # Example
///
/// ```rust,ignore
/// use miral::prelude::*;
/// use std::sync::Arc;
///
/// let handle = Arc::new(std::sync::Mutex::new(None));
/// let handle_clone = handle.clone();
///
/// MirRunner::new(std::env::args())
///     .add_window_management_policy::<MyPolicy>()
///     .on_start(move || {
///         // Store a stop handle for later
///     })
///     .run()
///     .expect("Server failed");
/// ```
#[derive(Clone)]
pub struct RunnerHandle {
    runner_ptr: Arc<std::sync::atomic::AtomicU64>,
}

// Safety: The underlying MirRunner::stop() is thread-safe in Mir.
unsafe impl Send for RunnerHandle {}
unsafe impl Sync for RunnerHandle {}

impl RunnerHandle {
    /// Create a new runner handle from a raw pointer.
    pub(crate) fn new(ptr: u64) -> Self {
        Self {
            runner_ptr: Arc::new(std::sync::atomic::AtomicU64::new(ptr)),
        }
    }

    /// Request the compositor to stop.
    ///
    /// This is safe to call from any thread. The server will shut down
    /// gracefully after this call.
    pub fn stop(&self) {
        let ptr = self.runner_ptr.load(std::sync::atomic::Ordering::Acquire);
        if ptr != 0 {
            // Safety: the pointer is valid while the server is running,
            // and stop() is thread-safe in Mir.
            unsafe {
                let runner = &mut *(ptr as *mut miral_sys::ffi::MiralRunner);
                let pinned = std::pin::Pin::new_unchecked(runner);
                miral_sys::ffi::miral_runner_stop(pinned);
            }
        }
    }
}

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
    idle_listener: Option<IdleListener>,
    session_lock_listener: Option<SessionLockListener>,
    magnifier: Option<Magnifier>,
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
            idle_listener: None,
            session_lock_listener: None,
            magnifier: None,
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

    /// Configure idle state callbacks for the compositor.
    pub fn idle_listener(mut self, listener: IdleListener) -> Self {
        self.idle_listener = Some(listener);
        self
    }

    /// Configure session lock and unlock callbacks for the compositor.
    pub fn session_lock_listener(mut self, listener: SessionLockListener) -> Self {
        self.session_lock_listener = Some(listener);
        self
    }

    /// Configure the compositor magnifier.
    pub fn magnifier(mut self, magnifier: Magnifier) -> Self {
        self.magnifier = Some(magnifier);
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
        let MirRunner {
            args,
            extensions,
            config_options,
            policy_factory,
            on_start,
            on_stop,
            idle_listener,
            session_lock_listener,
            magnifier,
        } = self;

        let policy_factory = policy_factory.ok_or(
            "No window management policy set. Call add_window_management_policy() before run().",
        )?;

        miral_sys::set_policy_factory(Box::new(policy_factory));

        let mut decoration_mode: i32 = 0;
        let mut keymap_layout = String::new();
        let mut x11_enabled = false;
        let mut external_launcher_enabled = false;

        for ext in &extensions {
            match ext.name() {
                "Decorations(PreferCSD)" => decoration_mode = 1,
                "Decorations(PreferSSD)" => decoration_mode = 2,
                "Decorations(AlwaysSSD)" => decoration_mode = 3,
                "Decorations(AlwaysCSD)" => decoration_mode = 4,
                "Keymap" => {}
                "X11Support" => x11_enabled = true,
                "ExternalClientLauncher" => external_launcher_enabled = true,
                _ => {}
            }
        }

        for ext in &extensions {
            if let Some(keymap) = ext.as_any().downcast_ref::<Keymap>() {
                keymap_layout = if let Some(variant) = keymap.variant() {
                    format!("{}({})", keymap.layout(), variant)
                } else {
                    keymap.layout().to_string()
                };
            }
        }

        let mut runner = miral_sys::ffi::miral_runner_new(&args);

        if external_launcher_enabled {
            miral_sys::ffi::miral_runner_enable_external_launcher(runner.pin_mut());
        }

        if let Some(idle_listener) = idle_listener {
            let IdleListener {
                on_dim,
                on_off,
                on_wake,
            } = idle_listener;
            let has_idle_callbacks = on_dim.is_some() || on_off.is_some() || on_wake.is_some();

            if let Some(callback) = on_dim {
                miral_sys::set_on_idle_dim_callback(callback);
            }
            if let Some(callback) = on_off {
                miral_sys::set_on_idle_off_callback(callback);
            }
            if let Some(callback) = on_wake {
                miral_sys::set_on_idle_wake_callback(callback);
            }
            if has_idle_callbacks {
                miral_sys::ffi::miral_runner_enable_idle_listener(runner.pin_mut());
            }
        }

        if let Some(session_lock_listener) = session_lock_listener {
            miral_sys::set_on_session_lock_callback(session_lock_listener.on_lock);
            miral_sys::set_on_session_unlock_callback(session_lock_listener.on_unlock);
            miral_sys::ffi::miral_runner_enable_session_lock_listener(runner.pin_mut());
        }

        if let Some(magnifier) = magnifier {
            miral_sys::ffi::miral_runner_enable_magnifier(
                runner.pin_mut(),
                magnifier.magnification,
                magnifier.capture_size.width,
                magnifier.capture_size.height,
                magnifier.enabled,
            );
        }

        let runner_ptr = {
            let pinned = runner.pin_mut();
            unsafe { pinned.get_unchecked_mut() as *mut _ as u64 }
        };
        let runner_handle = RunnerHandle::new(runner_ptr);

        if let Some(on_start) = on_start {
            miral_sys::set_on_start_callback(Box::new(on_start));
            miral_sys::ffi::miral_runner_register_start_callback(runner.pin_mut());
        }
        if let Some(on_stop) = on_stop {
            miral_sys::set_on_stop_callback(Box::new(on_stop));
            miral_sys::ffi::miral_runner_register_stop_callback(runner.pin_mut());
        }

        let handle_for_cleanup = runner_handle.clone();

        let mut config_descs = Vec::new();
        for option in config_options {
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

        let result = miral_sys::ffi::miral_runner_run_with_config(
            runner.pin_mut(),
            decoration_mode,
            &keymap_layout,
            x11_enabled,
            &config_descs,
        );

        handle_for_cleanup
            .runner_ptr
            .store(0, std::sync::atomic::Ordering::Release);

        miral_sys::clear_config_callbacks();
        miral_sys::clear_runner_callbacks();

        if result != 0 {
            Err(format!("Server exited with code {}", result).into())
        } else {
            Ok(())
        }
    }

    /// Get a handle that can be used to stop the compositor from another thread.
    ///
    /// Note: This method must be called before `run()`. The handle is only valid
    /// while the compositor is running. Consider using [`on_start`](Self::on_start)
    /// to obtain a [`RunnerHandle`] at the right time.
    ///
    /// For most use cases, call `stop()` from within a signal handler or
    /// a spawned thread:
    ///
    /// ```rust,ignore
    /// use miral::runner::{MirRunner, RunnerHandle};
    /// use std::sync::{Arc, Mutex};
    ///
    /// let handle: Arc<Mutex<Option<RunnerHandle>>> = Arc::new(Mutex::new(None));
    /// let h = handle.clone();
    ///
    /// MirRunner::new(std::env::args())
    ///     .add_window_management_policy::<MyPolicy>()
    ///     .on_start(move || {
    ///         // Use handle to stop later if needed
    ///     })
    ///     .run();
    /// ```
    pub fn runner_handle(&self) -> RunnerHandle {
        // Returns a handle with null pointer - will be set when run() is called
        RunnerHandle::new(0)
    }
}
