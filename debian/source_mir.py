from apport.hookutils import *

def add_info(report, ui=None):
    attach_file_if_exists(report, '/var/log/boot.log', 'BootLog')

    report['version.libdrm'] = package_versions('libdrm2')
    report['version.lightdm'] = package_versions('lightdm')
    report['version.mesa'] = package_versions('libegl1-mesa-dev')
    return True

if __name__ == '__main__':
    import sys

    report = {}
    if not add_info(report, None):
        print("Unreportable")
        sys.exit(1)
    keys = report.keys()
    keys.sort()
    for key in keys:
        print("[%s]\n%s\n" %(key, report[key]))
