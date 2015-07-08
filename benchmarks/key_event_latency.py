#!/usr/bin/python3

from mir_perf_framework import PerformanceTest, Server, Client
import time
import evdev
import statistics

####### TEST #######

host = Server()
nested = Server(host=host, reports=["client-input-receiver"])
client = Client(server=nested, reports=["client-input-receiver"])

ui = evdev.UInput()
test = PerformanceTest([host, nested, client])

test.start()

for i in range(1000):
    ui.write(evdev.ecodes.EV_KEY, evdev.ecodes.KEY_A, 1)
    ui.syn()
    time.sleep(0.002)
    ui.write(evdev.ecodes.EV_KEY, evdev.ecodes.KEY_A, 0)
    ui.syn()
    time.sleep(0.002)

test.stop()

####### TRACE PARSING #######

trace = test.babeltrace()

pids = {}
data = {}

for event in trace.events:
    if event.name == "mir_client_input_receiver:key_event":
        pid = event["vpid"]
        if pid not in pids.values():
            if "nested" not in pids:
                pids["nested"] = pid
            elif "client" not in pids:
                pids["client"] = pid
        if pid not in data: data[pid] = []

        data[pid].append((event.timestamp - event["event_time"]) / 1000000.0)


print("=== Results ===")

nested_data = data[pids["nested"]]
print("Kernel to nested mean: %f ms stdev: %f ms" %
      (statistics.mean(nested_data), statistics.stdev(nested_data)))

client_data = data[pids["client"]]
print("Kernel to client mean: %f ms stdev: %f ms" %
      (statistics.mean(client_data), statistics.stdev(client_data)))
