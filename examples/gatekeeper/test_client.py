#!/usr/bin/env python3
import sys
import gi
import signal

gi.require_version('Gio', '2.0')
from gi.repository import Gio, GLib

# XML for the interface we (the client) will expose to receive events
# [FIXED] Removed session_handle argument to match Gatekeeper's call signature (sua{sv})
CLIENT_XML = """
<node>
  <interface name="org.freedesktop.portal.GlobalShortcuts">
    <method name="Activated">
      <arg type="s" name="shortcut_id" direction="in"/>
      <arg type="u" name="timestamp" direction="in"/>
      <arg type="a{sv}" name="options" direction="in"/>
    </method>
    <method name="Deactivated">
      <arg type="s" name="shortcut_id" direction="in"/>
      <arg type="u" name="timestamp" direction="in"/>
      <arg type="a{sv}" name="options" direction="in"/>
    </method>
  </interface>
</node>
"""

class TestClient:
    def __init__(self):
        self.app_id = "org.example.TestClient"
        self.bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)

        # Connect to the Gatekeeper implementation directly
        self.proxy = Gio.DBusProxy.new_sync(
            self.bus,
            Gio.DBusProxyFlags.NONE,
            None,
            "org.freedesktop.impl.portal.desktop.mir",
            "/org/freedesktop/portal/desktop",
            "org.freedesktop.impl.portal.GlobalShortcuts",
            None
        )

        # Subscribe to signals to detect when shortcuts are changed by the user
        self.bus.signal_subscribe(
            "org.freedesktop.impl.portal.desktop.mir",
            "org.freedesktop.impl.portal.GlobalShortcuts",
            "ShortcutsChanged",
            "/org/freedesktop/portal/desktop",
            None,
            Gio.DBusSignalFlags.NONE,
            self.on_shortcuts_changed,
            None
        )

        # Setup listener for Activated/Deactivated events
        self.setup_client_listener()

    def setup_client_listener(self):
        # We simply register the object. Gatekeeper knows our bus unique name (e.g. :1.99)
        # because it captured it when we called CreateSession.
        session_path = "/org/freedesktop/portal/desktop/session/test_client/1"
        node_info = Gio.DBusNodeInfo.new_for_xml(CLIENT_XML)
        self.bus.register_object(
            session_path,
            node_info.interfaces[0],
            self.on_client_method_call,
            None,
            None
        )

    def on_client_method_call(self, connection, sender, object_path, interface_name, method_name, parameters, invocation):
        # Handle the callbacks from Gatekeeper
        if method_name == "Activated":
            # [FIXED] Unpack 3 arguments: shortcut_id, timestamp, options
            s_id, timestamp, options = parameters.unpack()
            print(f"🔥 [Event] Activated: {s_id} (Time: {timestamp})")
            invocation.return_value(None)

        elif method_name == "Deactivated":
            # [FIXED] Unpack 3 arguments
            s_id, timestamp, options = parameters.unpack()
            print(f"❄️  [Event] Deactivated: {s_id} (Time: {timestamp})")
            invocation.return_value(None)

    def on_shortcuts_changed(self, connection, sender_name, object_path, interface_name, signal_name, parameters, user_data):
        session_handle, shortcuts = parameters.unpack()
        print(f"\n[Signal] ShortcutsChanged for session {session_handle}")
        for s_id, data in shortcuts:
            print(f"  Shortcut '{s_id}' changed:")
            print(f"    Data: {data}")

    def run(self):
        print("Creating session...")
        session_handle = "/org/freedesktop/portal/desktop/session/test_client/1"
        request_handle = "/org/freedesktop/portal/desktop/request/test_client/1"

        try:
            ret = self.proxy.call_sync(
                "CreateSession",
                GLib.Variant("(oosa{sv})", (
                    request_handle,
                    session_handle,
                    self.app_id,
                    {}
                )),
                Gio.DBusCallFlags.NONE,
                -1,
                None
            )
            response, results = ret.unpack()
            if response != 0:
                print(f"CreateSession failed with response {response}")
                return
            print("Session created successfully.")
        except Exception as e:
            print(f"Error calling CreateSession: {e}")
            print("Make sure gatekeeper.py is running.")
            return

        print("Binding shortcuts...")
        shortcuts = [
            ("screenshot", {
                "description": GLib.Variant("s", "Take a screenshot"),
                "preferred_trigger": GLib.Variant("s", "<Control><Alt>s")
            }),
            ("quit", {
                "description": GLib.Variant("s", "Quit Application"),
                "preferred_trigger": GLib.Variant("s", "<Control>q")
            })
        ]

        request_handle_bind = "/org/freedesktop/portal/desktop/request/test_client/2"

        try:
            print("Please check the Gatekeeper window to confirm shortcuts...")
            ret = self.proxy.call_sync(
                "BindShortcuts",
                GLib.Variant("(ooa(sa{sv})sa{sv})", (
                    request_handle_bind,
                    session_handle,
                    shortcuts,
                    "", # parent_window
                    {}
                )),
                Gio.DBusCallFlags.NONE,
                -1,
                None
            )
            response, results = ret.unpack()
            if response != 0:
                print(f"BindShortcuts failed with response {response}")
                return

            print("BindShortcuts successful!")
            print("Registered Shortcuts:")
            shortcuts_list = results.get('shortcuts', [])
            for s_id, data in shortcuts_list:
                print(f"  ID: {s_id}")
                print(f"  Token: {data.get('trigger_action_token', 'N/A')}")

        except Exception as e:
            print(f"Error calling BindShortcuts: {e}")
            return

        print("\nListening for updates (Ctrl+C to exit)...")
        loop = GLib.MainLoop()
        try:
            loop.run()
        except KeyboardInterrupt:
            pass

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    client = TestClient()
    client.run()
