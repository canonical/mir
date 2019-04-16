#!/usr/bin/env python3

import babeltrace
import sys
from text_histogram import histogram

howto = """
Process a LTTNG trace for per-client commit-to-present latencies.

This works on an lttng trace with the mir_server_wayland:sw_buffer_committed
and mir_server_compositor:buffers_in_frame tracepoints enabled. To generate
such a trace you need to run a Mir server with the --compositor-report=lttng
option. For example:

> miral-app --compositor-report=lttng

Once you have your compositor active, you can get an LTTNG trace like so:
> lttng create mir-trace
> lttng enable-event -u "mir_server_wayland:sw_buffer_committed"
> lttng enable-event -u "mir_server_compositor:buffers_in_frame"
> lttng start

At this point you can interact with applications in your Mir session;
LTTNG will be recording the necessary events. When you're done…

> lttng stop
> lttng destroy

This will (by default) leave you with a trace in
~/lttng-traces/mir-trace-DIGITS-MORE_DIGITS/ust/uid/$YOUR_UID/64-bit

This trace can then be processed with
> python3 commit_to_present.py ~/lttng-traces/mir-trace-$DIGITS-$MORE_DIGITS/ust/uid/$YOUR_UID/64-bit
"""

try:
    trace_path = sys.argv[1]

    trace_collection = babeltrace.TraceCollection()
    trace_collection.add_trace(trace_path, 'ctf')
except:
    print(howto)
    raise

class ClientStats:
    def __init__(self):
        self.buffers_in_flight = dict()
        self.buffer_delays = []

clients = dict()
for event in trace_collection.events:
    if event.name == 'mir_server_wayland:sw_buffer_committed':
        if event['client'] not in clients:
            clients[event['client']] = ClientStats()
        clients[event['client']].buffers_in_flight[event['buffer_id']] = event.timestamp
    if event.name == 'mir_server_compositor:buffers_in_frame':
        for buffer in event['buffer_ids']:
            for client in clients.values():
                if buffer in client.buffers_in_flight.keys():
                    client.buffer_delays.append(event.timestamp - client.buffers_in_flight[buffer])
                    del client.buffers_in_flight[buffer]

for (id, client)  in clients.items():
    print("Client: ", id)
    histogram([delay // 1000000 for delay in client.buffer_delays])
