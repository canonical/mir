from apport.hookutils import *

def add_info(report, ui=None):
    attach_related_packages(report, ['lightdm'])
    attach_file_if_exists(report, '/var/log/boot.log', 'BootLog')
