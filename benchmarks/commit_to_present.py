#!/usr/bin/env python3

import babeltrace
import sys
from text_histogram import histogram

trace_path = sys.argv[1]

trace_collection = babeltrace.TraceCollection()
trace_collection.add_trace(trace_path, 'ctf')

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
