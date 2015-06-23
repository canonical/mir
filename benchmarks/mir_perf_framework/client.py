import os
import subprocess
import shutil
from .common import env_variable_for_client_report

class Client:
    def __init__(self, executable=shutil.which("mir_demo_client_egltriangle"), server=None, reports=[]):
        self.executable = executable
        self.server = server
        self.reports = reports
        self.process = None

    def start(self):
        cmdline = [self.executable, "-m", self.server.socket_path]
        env = os.environ.copy()
        env["LTTNG_UST_WITHOUT_BADDR_STATEDUMP"] = "1"
        for report in self.reports:
            env[env_variable_for_client_report(report)] = "lttng"

        self.process = subprocess.Popen(cmdline, env=env)

    def stop(self):
        self.process.terminate()
