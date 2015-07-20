#!/usr/bin/python3

from mir_perf_framework import PerformanceTest, Server, Client
import time
import statistics
import itertools

####### TEST #######

host = Server(reports=["display", "compositor"])
nested = Server(host=host, reports=["client-perf", "compositor"])
client = Client(server=nested, reports=["client-perf"])

test = PerformanceTest([host, nested, client])

test.start()

time.sleep(5)

test.stop()

####### TRACE PARSING #######

class Frame:
    def __init__(self, events, pids, event_index):
        self.events = events
        self.pids = pids
        self.event_index = event_index

    def find_frame_containing_this_one(self, pid, frame_end_event):
        frame = None
        buffer_in_frame = False

        for i, event in enumerate(itertools.islice(self.events, self.event_index, len(self.events)), start=self.event_index):
            if event["vpid"] != pid: continue
            if event.name == frame_end_event and buffer_in_frame:
                frame = Frame(self.events, self.pids, i)
                break
            if event.name == "mir_server_compositor:buffers_in_frame" and self.buffer_id() in event["buffer_ids"]:
                buffer_in_frame = True

        return frame

    def find_nested_frame_containing_this_one(self):
        return self.find_frame_containing_this_one(pids["nested"], "mir_client_perf:end_frame")

    def find_host_frame_containing_this_one(self):
        return self.find_frame_containing_this_one(pids["host"], "mir_server_display:report_vsync")

    def buffer_id(self):
        return self.events[self.event_index]["buffer_id"]

    def timestamp(self):
        return self.events[self.event_index].timestamp

def find_pids(events):
    pids = {}

    for event in events:
        if event.name == "mir_server_compositor:started":
            pid = event["vpid"]
            if "host" not in pids:
                pids["host"] = pid
            else:
                pids["nested"] = pid
                break

    for event in events:
        pid = event["vpid"]
        if pid not in pids.values():
            pids["client"] = pid
            break

    return pids

def find_client_frame_events(events, pids):
    client_frames = []
    for i, event in enumerate(events):
        if event["vpid"] == pids["client"] and event.name == "mir_client_perf:end_frame":
            client_frames.append(Frame(events, pids, i))
    return client_frames

events = test.events()
pids = find_pids(events)
client_frame_events = find_client_frame_events(events, pids)
data = []

for client_frame in client_frame_events:
    nested_frame = client_frame.find_nested_frame_containing_this_one()
    if nested_frame is None: continue

    host_frame = nested_frame.find_host_frame_containing_this_one()
    if host_frame is None: continue

    data.append((host_frame.timestamp() - client_frame.timestamp()) / 1000000.0)

print("=== Results ===")
print("Tracked %d buffers from nested client to display" % len(data))
print("Latency mean: %f ms stdev: %f ms" %
      (statistics.mean(data), statistics.stdev(data)))
