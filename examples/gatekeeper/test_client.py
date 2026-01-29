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
        print(f"📡 Registering D-Bus object at path: {session_path}")
        print(f"   This allows Gatekeeper to send Activated/Deactivated events back to us")

        node_info = Gio.DBusNodeInfo.new_for_xml(CLIENT_XML)
        try:
            registration_id = self.bus.register_object(
                session_path,
                node_info.interfaces[0],
                self.on_client_method_call,
                None,
                None
            )
            print(f"✅ Successfully registered D-Bus object (ID: {registration_id})")
        except Exception as e:
            print(f"❌ Failed to register D-Bus object: {e}")
            raise

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
        print("=" * 60)
        print("🚀 Starting Test Client")
        print("=" * 60)

        print("\n📞 Step 1: Creating session...")
        print(f"   App ID: {self.app_id}")
        session_handle = "/org/freedesktop/portal/desktop/session/test_client/1"
        request_handle = "/org/freedesktop/portal/desktop/request/test_client/1"
        print(f"   Session handle: {session_handle}")
        print(f"   Request handle: {request_handle}")

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
                GLib.MAXINT,
                None
            )
            response, results = ret.unpack()
            if response != 0:
                print(f"❌ CreateSession failed with response code: {response}")
                return
            print(f"✅ Session created successfully!")
            print(f"   Response: {response}, Results: {results}")
        except Exception as e:
            print(f"❌ Error calling CreateSession: {e}")
            print("   Make sure gatekeeper.py is running.")
            return

        print("\n⌨️  Step 2: Binding shortcuts...")
        shortcuts = [
            ("screenshot", {
                "description": GLib.Variant("s", "Take a screenshot"),
                "preferred_trigger": GLib.Variant("s", "<Control><Alt>s")
            }),
            ("quit", {
                "description": GLib.Variant("s", "Quit Application"),
                "preferred_trigger": GLib.Variant("s", "<Control>q")
            }),
            ("duplicate-quit", {
                "description": GLib.Variant("s", "Quit Application"),
                "preferred_trigger": GLib.Variant("s", "<Control>q")
            })
        ]

        print(f"   Requesting shortcuts:")
        for shortcut_id, data in shortcuts:
            desc = data['description'].get_string()
            trigger = data['preferred_trigger'].get_string()
            print(f"   - '{shortcut_id}': {desc} ({trigger})")

        request_handle_bind = "/org/freedesktop/portal/desktop/request/test_client/2"
        print(f"   Request handle: {request_handle_bind}")

        try:
            print("\n⏳ Waiting for user confirmation in Gatekeeper window...")
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
                GLib.MAXINT,
                None
            )
            response, results = ret.unpack()

            print(f"\n📋 BindShortcuts response received:")
            print(f"   Response code: {response}")

            if response != 0:
                print(f"❌ BindShortcuts failed with response code: {response}")
                print(f"   (0 = success, 1 = user cancelled, 2 = other error)")
                return

            print(f"✅ BindShortcuts successful!")
            print(f"\n📝 Registered Shortcuts:")
            shortcuts_list = results.get('shortcuts', [])
            if shortcuts_list:
                for s_id, data in shortcuts_list:
                    print(f"   ✓ ID: '{s_id}'")
                    token = data.get('trigger_action_token')
                    if token:
                        print(f"     Token: {token}")
                    trigger = data.get('trigger_description')
                    if trigger:
                        print(f"     Trigger: {trigger}")
            else:
                print("   (No shortcuts returned in results)")

        except Exception as e:
            print(f"❌ Error calling BindShortcuts: {e}")
            return

        print("\n" + "=" * 60)
        print("👂 Listening for shortcut events (Ctrl+C to exit)...")
        print("=" * 60)
        print("Press your registered shortcuts to see Activated/Deactivated events")
        print()

        loop = GLib.MainLoop()
        try:
            loop.run()
        except KeyboardInterrupt:
            print("\n\n🛑 Shutting down...")

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    client = TestClient()
    client.run()
