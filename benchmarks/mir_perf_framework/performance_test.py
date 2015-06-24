import subprocess
import babeltrace
import os
import tempfile
import atexit
from .common import unique_lttng_session_name

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

        for dirname, subdirs, files in os.walk(self.lttng.output_dir):
            if 'metadata' in files:
                col.add_trace(dirname, 'ctf')

        return col
