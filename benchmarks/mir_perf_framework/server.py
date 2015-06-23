import os
import time
import subprocess
import shutil
from .common import env_variable_for_client_report, unique_server_socket_path

class Server:
    def __init__(self, executable=shutil.which("mir_demo_server"), host=None, reports=[]):
        self.executable = executable
        self.host = host
        self.reports = reports
        self.socket_path = unique_server_socket_path()
        self.process = None

    def start(self):
        cmdline = [self.executable, "-f", self.socket_path]
        if self.host is not None:
            cmdline.extend(["--host-socket", self.host.socket_path])

        env = os.environ.copy()
        env["LTTNG_UST_WITHOUT_BADDR_STATEDUMP"] = "1"

        for report in self.reports:
            if report.startswith("client-"):
                env[env_variable_for_client_report(report)] = "lttng"
            else:
                cmdline.append("--%s-report=lttng" % report)

        self.process = subprocess.Popen(cmdline, env=env)
        self.wait_for_socket()

    def stop(self):
        self.process.terminate()

    def wait_for_socket(self):
        while not os.path.exists(self.socket_path):
            time.sleep(0.01)
