import tempfile
import os
import random
import string

random_str = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(6))

def env_variable_for_client_report(i):
    return "MIR_%s_REPORT" % i.upper().replace("-", "_")

def unique_server_socket_path():
    unique_server_socket_path.server_count += 1
    tmpname = "mir_perf_socket_" + str(os.getppid()) + "_"
    tmpname += str(random_str) + "_"
    tmpname += str(unique_server_socket_path.server_count)
    return os.path.join(tempfile.gettempdir(), tmpname)

unique_server_socket_path.server_count = 0

def unique_lttng_session_name():
    return "mir_perf_lttng_" + str(os.getpid()) + "_" + random_str
