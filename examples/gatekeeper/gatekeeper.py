#!/usr/bin/env python3
import sys
import os
import gi
import logging

gi.require_version('Gtk', '4.0')
from gi.repository import Gtk, GLib, Gio, Gdk

from pywayland.client import Display
from pywayland.protocol.wayland import WlRegistry

# Add current directory to path to find generated protocols
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

try:
    from protocols.ext_input_trigger_registration_v1 import ExtInputTriggerRegistrationManagerV1
    from protocols.ext_input_trigger_action_v1 import ExtInputTriggerActionManagerV1, ExtInputTriggerActionV1
except ImportError:
    print("Error: Could not import generated protocols. Please run generate_protocols.py first.")
    sys.exit(1)

# Setup logging - Updated to DEBUG for verbose output
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("Gatekeeper")

# DBus Interface XML
GLOBAL_SHORTCUTS_XML = """
<node>
  <interface name="org.freedesktop.impl.portal.GlobalShortcuts">
    <method name="CreateSession">
      <arg type="o" name="request_handle" direction="in"/>
      <arg type="o" name="session_handle" direction="in"/>
      <arg type="s" name="app_id" direction="in"/>
      <arg type="a{sv}" name="options" direction="in"/>
      <arg type="u" name="response" direction="out"/>
      <arg type="a{sv}" name="results" direction="out"/>
    </method>
    <method name="BindShortcuts">
      <arg type="o" name="request_handle" direction="in"/>
      <arg type="o" name="session_handle" direction="in"/>
      <arg type="a(sa{sv})" name="shortcuts" direction="in"/>
      <arg type="s" name="parent_window" direction="in"/>
      <arg type="a{sv}" name="options" direction="in"/>
      <arg type="u" name="response" direction="out"/>
      <arg type="a{sv}" name="results" direction="out"/>
    </method>
    <method name="ListShortcuts">
      <arg type="o" name="request_handle" direction="in"/>
      <arg type="o" name="session_handle" direction="in"/>
      <arg type="u" name="response" direction="out"/>
      <arg type="a{sv}" name="results" direction="out"/>
    </method>
    <signal name="ShortcutsChanged">
      <arg type="o" name="session_handle"/>
      <arg type="a(sa{sv})" name="shortcuts"/>
    </signal>
  </interface>
</node>
"""

# Minimal Keysym definitions (XKB)
KEY_A = 0x0061
# ... add more as needed

MOD_SHIFT = 0x08
MOD_CTRL = 0x100
MOD_ALT = 0x01
MOD_META = 0x800

def parse_accelerator(accel_string):
    mods = 0
    keyval = 0
    valid, keyval, modifiers = Gtk.accelerator_parse(accel_string)
    if not valid:
        return 0, 0
    if modifiers & Gdk.ModifierType.SHIFT_MASK:
        mods |= MOD_SHIFT
    if modifiers & Gdk.ModifierType.CONTROL_MASK:
        mods |= MOD_CTRL
    if modifiers & Gdk.ModifierType.ALT_MASK:
        mods |= MOD_ALT
    if modifiers & Gdk.ModifierType.META_MASK:
        mods |= MOD_META
    return mods, keyval

class ShortcutDialog(Gtk.Dialog):
    def __init__(self, parent, shortcuts):
        super().__init__(title="Register Shortcuts", transient_for=parent, modal=True)
        self.shortcuts = shortcuts
        self.entries = {}

        box = self.get_content_area()
        box.set_spacing(10)
        box.set_margin_top(10)
        box.set_margin_bottom(10)
        box.set_margin_start(10)
        box.set_margin_end(10)

        lbl = Gtk.Label(label="An application wants to register the following shortcuts:")
        box.append(lbl)

        grid = Gtk.Grid()
        grid.set_row_spacing(6)
        grid.set_column_spacing(12)
        box.append(grid)

        for i, (s_id, options) in enumerate(shortcuts):
            desc = options.get('description', s_id)
            grid.attach(Gtk.Label(label=desc, xalign=0), 0, i, 1, 1)
            entry = Gtk.Entry()
            entry.set_placeholder_text("<Control>a")
            preferred = options.get('preferred_trigger', '')
            if preferred:
                entry.set_text(preferred)
            self.entries[s_id] = entry
            grid.attach(entry, 1, i, 1, 1)

        self.add_button("Cancel", Gtk.ResponseType.CANCEL)
        self.add_button("Register", Gtk.ResponseType.OK)

    def get_triggers(self):
        triggers = {}
        for s_id, entry in self.entries.items():
            text = entry.get_text()
            if text:
                triggers[s_id] = text
        return triggers

class GatekeeperWindow(Gtk.ApplicationWindow):
    def __init__(self, app):
        super().__init__(application=app, title="Gatekeeper")
        self.app = app
        self.set_default_size(600, 400)
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        self.set_child(box)
        header = Gtk.HeaderBar()
        self.set_titlebar(header)
        label = Gtk.Label(label="Registered Shortcuts")
        label.set_margin_top(10)
        box.append(label)
        self.list_box = Gtk.ListBox()
        self.list_box.set_selection_mode(Gtk.SelectionMode.NONE)
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_child(self.list_box)
        scrolled.set_vexpand(True)
        box.append(scrolled)
        self.refresh_list()

    def refresh_list(self):
        while child := self.list_box.get_first_child():
            self.list_box.remove(child)

        for session_path, session in self.app.portal.sessions.items():
            app_id = session['app_id']
            for s_id, s_data in session['shortcuts'].items():
                row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
                row.set_margin_start(10)
                row.set_margin_end(10)
                row.set_margin_top(5)
                row.set_margin_bottom(5)
                info_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
                info_box.append(Gtk.Label(label=f"<b>{s_data.get('description', s_id)}</b>", use_markup=True, xalign=0))
                info_box.append(Gtk.Label(label=f"<small>{app_id} ({s_id})</small>", use_markup=True, xalign=0))
                info_box.set_hexpand(True)
                row.append(info_box)
                trigger_desc = s_data.get('trigger_desc', 'None')
                row.append(Gtk.Label(label=trigger_desc))
                edit_btn = Gtk.Button(label="Edit")
                edit_btn.connect("clicked", self.on_edit_clicked, session_path, s_id)
                row.append(edit_btn)
                self.list_box.append(row)

    def on_edit_clicked(self, btn, session_path, s_id):
        session = self.app.portal.sessions.get(session_path)
        if not session: return
        s_data = session['shortcuts'].get(s_id)
        if not s_data: return
        dialog = Gtk.Dialog(title="Edit Shortcut", transient_for=self, modal=True)
        dialog.add_button("Cancel", Gtk.ResponseType.CANCEL)
        dialog.add_button("Save", Gtk.ResponseType.OK)
        content = dialog.get_content_area()
        content.set_spacing(10)
        content.set_margin_top(10)
        content.set_margin_bottom(10)
        content.set_margin_start(10)
        content.set_margin_end(10)
        entry = Gtk.Entry()
        entry.set_text(s_data.get('trigger_desc', ''))
        content.append(Gtk.Label(label=f"New trigger for {s_data.get('description', s_id)}:"))
        content.append(entry)
        dialog.connect("response", self.on_edit_response, session_path, s_id, entry)
        dialog.show()

    def on_edit_response(self, dialog, response_id, session_path, s_id, entry):
        dialog.destroy()
        if response_id == Gtk.ResponseType.OK:
            new_trigger = entry.get_text()
            self.app.update_shortcut(session_path, s_id, new_trigger)

class GlobalShortcutsPortal:
    def __init__(self, app):
        self.app = app
        self.sessions = {} # session_handle -> { 'app_id': str, 'shortcuts': dict, 'sender': str }

    # [CHANGE] Added sender argument
    def CreateSession(self, request_handle, session_handle, app_id, options, sender):
        logger.info(f"CreateSession: {session_handle} for {app_id} (Sender: {sender})")
        self.sessions[session_handle] = {
            'app_id': app_id,
            'shortcuts': {},
            'sender': sender  # [CHANGE] Store the sender
        }
        return 0, {}

    def ListShortcuts(self, request_handle, session_handle):
        if session_handle not in self.sessions:
            return 2, {}
        shortcuts = self.sessions[session_handle]['shortcuts']
        result_shortcuts = []
        for s_id, s_data in shortcuts.items():
            result_shortcuts.append((s_id, {'description': GLib.Variant('s', s_data.get('description', ''))}))
        return 0, {'shortcuts': GLib.Variant('a(sa{sv})', result_shortcuts)}

class GatekeeperApp(Gtk.Application):
    def __init__(self):
        super().__init__(application_id="org.freedesktop.impl.portal.desktop.mir.Gatekeeper",
                         flags=Gio.ApplicationFlags.HANDLES_COMMAND_LINE)
        self.display = None
        self.registry = None
        self.trigger_manager: ExtInputTriggerRegistrationManagerV1 = None
        self.action_manager: ExtInputTriggerActionManagerV1 = None
        self.portal = None
        self.dbus_con = None
        self.dbus_id = None
        self.win = None
        self.action: ExtInputTriggerActionV1 = None

    def do_startup(self):
        Gtk.Application.do_startup(self)
        self.setup_wayland()
        self.setup_dbus()
        self.hold()
        print("Gatekeeper running in background. Use --show-shortcuts to view UI.")

    def do_activate(self):
        if not self.win:
            self.win = GatekeeperWindow(self)
            self.win.connect("destroy", self.on_window_destroy)
        self.win.present()
        print("Gatekeeper UI shown.")

    def on_window_destroy(self, win):
        self.win = None

    def do_command_line(self, command_line):
        args = command_line.get_arguments()
        if len(args) > 1:
            if args[1] == "--show-shortcuts":
                self.activate()
                return 0
            print(f"Usage: {args[0]} [--show-shortcuts]", file=sys.stderr)
            return 1
        return 0

    def setup_wayland(self):
        try:
            logger.info("Connecting to Wayland display...")
            self.display = Display()
            self.display.connect()
            logger.info("Wayland display connected. Fetching registry...")
            self.registry = self.display.get_registry()
            self.registry.dispatcher['global'] = self.registry_global
            self.display.dispatch(block=True)
            self.display.roundtrip()
            fd = self.display.get_fd()
            GLib.io_add_watch(fd, GLib.IO_IN, self.on_wayland_readable)
        except Exception as e:
            logger.error(f"Failed to setup Wayland: {e}")

    def registry_global(self, registry, id, interface, version):
        if interface == "ext_input_trigger_registration_manager_v1":
            logger.info(f"[Wayland] Binding {interface}")
            self.trigger_manager = registry.bind(id, ExtInputTriggerRegistrationManagerV1, version)
        elif interface == "ext_input_trigger_action_manager_v1":
            logger.info(f"[Wayland] Binding {interface}")
            self.action_manager = registry.bind(id, ExtInputTriggerActionManagerV1, version)

    def on_wayland_readable(self, source, condition):
        self.display.read()
        self.display.dispatch()
        return True

    def setup_dbus(self):
        self.portal = GlobalShortcutsPortal(self)
        self.dbus_con = Gio.bus_get_sync(Gio.BusType.SESSION, None)
        node_info = Gio.DBusNodeInfo.new_for_xml(GLOBAL_SHORTCUTS_XML)
        interface_info = node_info.interfaces[0]
        self.dbus_id = self.dbus_con.register_object(
            "/org/freedesktop/portal/desktop",
            interface_info,
            self.on_method_call,
            None, None
        )
        Gio.bus_own_name_on_connection(self.dbus_con, "org.freedesktop.impl.portal.desktop.mir", Gio.BusNameOwnerFlags.NONE, None, None)

    def on_method_call(self, connection, sender, object_path, interface_name, method_name, parameters, invocation):
        args = parameters.unpack()
        if method_name == "CreateSession":
            request_handle, session_handle, app_id, options = args
            # [CHANGE] Pass invocation.get_sender() to track who created the session
            res = self.portal.CreateSession(request_handle, session_handle, app_id, options, invocation.get_sender())
            invocation.return_value(GLib.Variant("(ua{sv})", res))
        elif method_name == "BindShortcuts":
            request_handle, session_handle, shortcuts, parent_window, options = args
            if not self.trigger_manager or not self.action_manager:
                logger.error("Input trigger protocols not available")
                invocation.return_value(GLib.Variant("(ua{sv})", (2, {})))
                return
            self.prompt_for_shortcuts(session_handle, shortcuts, invocation)
        elif method_name == "ListShortcuts":
            request_handle, session_handle = args
            res = self.portal.ListShortcuts(request_handle, session_handle)
            invocation.return_value(GLib.Variant("(ua{sv})", res))

    def prompt_for_shortcuts(self, session_handle, shortcuts, invocation):
        parent = self.win if self.win else None
        dialog = ShortcutDialog(parent, shortcuts)
        dialog.connect("response", self.on_dialog_response, session_handle, shortcuts, invocation)
        dialog.show()

    def on_dialog_response(self, dialog, response_id, session_handle, shortcuts, invocation):
        triggers = dialog.get_triggers()
        dialog.destroy()
        if response_id == Gtk.ResponseType.OK:
            self.register_shortcuts_sequence(session_handle, shortcuts, triggers, invocation)
        else:
            invocation.return_value(GLib.Variant("(ua{sv})", (1, {})))

    def register_shortcuts_sequence(self, session_handle, shortcuts, triggers, invocation):
        session = self.portal.sessions.get(session_handle)
        if not session:
            invocation.return_value(GLib.Variant("(ua{sv})", (2, {})))
            return

        # [CHANGE] Retrieve the client sender ID from the session
        client_sender_id = session.get('sender')

        pending = len(shortcuts)
        results = {}

        if pending == 0:
            invocation.return_value(GLib.Variant("(ua{sv})", (0, {'shortcuts': GLib.Variant('a(sa{sv})', [])})))
            return

        def on_one_done(s_id, success, token, trigger_desc, wayland_objects=None):
            nonlocal pending
            if success:
                logger.info(f"Successfully registered shortcut {s_id} with token {token}")
                results[s_id] = {'trigger_action_token': GLib.Variant('s', token)}

                desc = ""
                for sid, opts in shortcuts:
                    if sid == s_id:
                        desc = opts.get('description', '')
                        break

                action = self.action_manager.get_input_trigger_action(token)

                # [CHANGE] Use client_sender_id to direct the call to the specific app
                def on_action_begin(proxy, time, token):
                    logger.debug(f"Triggering Activated for {s_id} on {client_sender_id}")
                    if client_sender_id:
                        self.dbus_con.call(
                            client_sender_id,
                            session_handle,
                            "org.freedesktop.portal.GlobalShortcuts",
                            "Activated",
                            GLib.Variant("(sua{sv})", (s_id, time, {})),
                            GLib.VariantType.new("()"),
                            Gio.DBusCallFlags.NONE,
                            -1, None, None
                        )

                def on_action_end(proxy, time, token):
                    logger.debug(f"Triggering Deactivated for {s_id} on {client_sender_id}")
                    if client_sender_id:
                        self.dbus_con.call(
                            client_sender_id,
                            session_handle,
                            "org.freedesktop.portal.GlobalShortcuts",
                            "Deactivated",
                            GLib.Variant("(sua{sv})", (s_id, time, {})),
                            GLib.VariantType.new("()"),
                            Gio.DBusCallFlags.NONE,
                            -1, None, None
                        )

                action.dispatcher['begin'] = on_action_begin
                action.dispatcher['end'] = on_action_end

                self.display.roundtrip()
                wayland_objects['action'] = action

                session['shortcuts'][s_id] = {
                    'description': desc,
                    'trigger_desc': trigger_desc,
                    'token': token,
                    'wayland_objects': wayland_objects
                }
            else:
                logger.warning(f"Failed to register shortcut {s_id}")

            pending -= 1
            if pending == 0:
                shortcuts_list = []
                for sid, data in results.items():
                    shortcuts_list.append((sid, data))
                final_results = {'shortcuts': GLib.Variant('a(sa{sv})', shortcuts_list)}
                invocation.return_value(GLib.Variant("(ua{sv})", (0, final_results)))
                if self.win:
                    self.win.refresh_list()

        for s_id, options in shortcuts:
            trigger_str = triggers.get(s_id)
            if trigger_str:
                self.register_single_shortcut(s_id, options.get('description', ''), trigger_str, on_one_done)
            else:
                on_one_done(s_id, False, None, None)

    def register_single_shortcut(self, s_id, description, trigger_str, callback):
        mods, keyval = parse_accelerator(trigger_str)
        if not self.trigger_manager:
            logger.error("Trigger manager not available")
            callback(s_id, False, None, trigger_str)
            return

        logger.debug(f"[Wayland] Registering trigger for '{s_id}'. Mods: {hex(mods)}, Key: {hex(keyval)}")
        trigger = self.trigger_manager.register_keyboard_sym_trigger(mods, keyval)
        context = {
            's_id': s_id,
            'description': description,
            'trigger': trigger,
            'trigger_str': trigger_str,
            'callback': callback
        }
        trigger.dispatcher['done'] = self.on_trigger_done
        trigger.dispatcher['failed'] = self.on_trigger_failed
        trigger.user_data = context
        self.display.roundtrip()

    def on_trigger_done(self, trigger):
        context = trigger.user_data
        action_control = self.trigger_manager.get_action_control(context['description'])
        context['action_control'] = action_control
        action_control.dispatcher['done'] = self.on_action_control_done
        action_control.user_data = context
        action_control.add_input_trigger_event(trigger)
        self.display.roundtrip()

    def on_action_control_done(self, action_control, token):
        context = action_control.user_data
        wayland_objects = {
            'trigger': context['trigger'],
            'action_control': action_control
        }
        context['callback'](context['s_id'], True, token, context['trigger_str'], wayland_objects)

    def on_trigger_failed(self, trigger):
        context = trigger.user_data
        logger.error(f"[Wayland] Trigger registration failed for {context['s_id']}")
        context['callback'](context['s_id'], False, None, context['trigger_str'])

    def update_shortcut(self, session_path, s_id, new_trigger_str):
        logger.info(f"Updating shortcut {s_id} for session {session_path} to {new_trigger_str}")
        session = self.portal.sessions.get(session_path)
        if session and s_id in session['shortcuts']:
            old_objects = session['shortcuts'][s_id].get('wayland_objects')
            if old_objects:
                if 'action_control' in old_objects: old_objects['action_control'].destroy()
                if 'trigger' in old_objects: old_objects['trigger'].destroy()

        def on_done(sid, success, token, trigger_desc, wayland_objects=None):
            if success:
                session = self.portal.sessions.get(session_path)
                if session:
                    session['shortcuts'][s_id]['trigger_desc'] = trigger_desc
                    session['shortcuts'][s_id]['token'] = token
                    session['shortcuts'][s_id]['wayland_objects'] = wayland_objects

                    changed = [(s_id, {'trigger_action_token': GLib.Variant('s', token)})]
                    logger.debug(f"Emitting ShortcutsChanged signal for {s_id}")
                    self.dbus_con.emit_signal(
                        None,
                        "/org/freedesktop/portal/desktop",
                        "org.freedesktop.impl.portal.GlobalShortcuts",
                        "ShortcutsChanged",
                        GLib.Variant("(oa(sa{sv}))", (session_path, changed))
                    )
                    if self.win:
                        self.win.refresh_list()

        session = self.portal.sessions.get(session_path)
        if session and s_id in session['shortcuts']:
            desc = session['shortcuts'][s_id]['description']
            self.register_single_shortcut(s_id, desc, new_trigger_str, on_done)

if __name__ == "__main__":
    app = GatekeeperApp()
    app.run(sys.argv)
