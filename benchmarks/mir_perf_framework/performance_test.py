import subprocess
import babeltrace
import os
import tempfile
import atexit
from .common import unique_lttng_session_name

class LttngEvent:
    def __init__(self, bt_event):
        self.name = bt_event.name
        self.timestamp = bt_event.timestamp
        self.fields = {}
        for (k,v) in bt_event.items():
            self.fields[k] = v

    def __getitem__(self, k):
        return self.fields.get(k, None)

    def __str__(self):
        ts = str(self.timestamp)
        ts = ts[:-9] + "." + ts[-9:]
        return "[%s] %s %s" % (ts, self.name, self.fields)

class Lttng:
    def __init__(self, keep_lttng_traces):
        self.session_name = unique_lttng_session_name()
        self.output_dir = os.path.join(tempfile.gettempdir(), self.session_name)
        self.keep_lttng_traces = keep_lttng_traces

    def cleanup(self):
        subprocess.call("rm -rf %s" % self.output_dir, shell=True)

    def start(self):
        # Ensure we start with a clean session
        subprocess.call(["lttng", "stop"])
        subprocess.call(["lttng", "destroy", self.session_name])
        subprocess.call("rm -rf %s" % self.output_dir, shell=True)
        if not self.keep_lttng_traces:
            atexit.register(self.cleanup)
        subprocess.call(["lttng", "create", self.session_name, "-o", self.output_dir])
        subprocess.call(["lttng", "enable-event", "-u", "-a"])
        subprocess.call(["lttng", "add-context", "-u", "-t", "vpid"])
        subprocess.call(["lttng", "start"])

    def stop(self):
        subprocess.call(["lttng", "stop"])
        subprocess.call(["lttng", "destroy", self.session_name])

class PerformanceTest:
    def __init__(self, components, keep_lttng_traces=False):
        self.components = components
        self.lttng = Lttng(keep_lttng_traces)

    def start(self):
        self.lttng.start()
        for component in self.components:
            component.start()

    def stop(self):
        for component in reversed(self.components):
            component.stop()
        self.lttng.stop()

    def babeltrace(self):
        col = babeltrace.TraceCollection()
        col.add_traces_recursive(self.lttng.output_dir, 'ctf')

        return col

    def events(self):
        trace = self.babeltrace()

        events = []
        for e in trace.events:
            events.append(LttngEvent(e))

        return events
