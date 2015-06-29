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

""" A performance test object """
class PerformanceTest:
    def __init__(self, components, keep_lttng_traces=False):
        """ Creates a PerformanceTest

        Args:
            components (list of Server and Client): An ordered list of test components to start.
            keep_lttng_traces (bool): Whether to keep the lttng trace directory produced by this run.
        """

        self.components = components
        self.lttng = Lttng(keep_lttng_traces)

    def start(self):
        """ Starts the performance test """
        self.lttng.start()
        for component in self.components:
            component.start()

    def stop(self):
        """ Stops the performance test"""
        for component in reversed(self.components):
            component.stop()
        self.lttng.stop()

    def babeltrace(self):
        """ Gets the babeltrace trace object for the test

        The test needs to have been stopped to get the trace.
        """
        col = babeltrace.TraceCollection()
        col.add_traces_recursive(self.lttng.output_dir, 'ctf')

        return col

    def events(self):
        """ Gets the trace events for the test

        The test needs to have been stopped to get the events.

        The events are returned in a list, making them easier to
        process than by using the raw babeltrace trace, which only
        supports iteration and access to one event at a time.
        """
        trace = self.babeltrace()

        events = []
        for e in trace.events:
            events.append(LttngEvent(e))

        return events
