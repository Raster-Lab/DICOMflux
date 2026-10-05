"""Check the probe export allowlist and native dependencies; no engine claims."""

def check(condition, message):
    if not condition:
        raise RuntimeError(str(message))
import argparse
import json
import sys
import platform
import subprocess
from pathlib import Path

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('library', type=Path)
    args = parser.parse_args()
    library = str(args.library.resolve())

    def call(command):
        return subprocess.check_output(command, text=True)
    if platform.system() == 'Darwin':
        exports = call(['nm', '-gU', library])
        dependencies = call(['otool', '-L', library])
        symbols = {line.split()[-1].removeprefix('_') for line in exports.splitlines() if line.split()}
    else:
        exports = call(['nm', '-D', '--defined-only', library])
        dependencies = call(['readelf', '-d', library])
        symbols = {line.split()[-1] for line in exports.splitlines() if line.split()}
    expected = {'dicomflux_probe_query_abi', 'dicomflux_probe_create', 'dicomflux_probe_measure', 'dicomflux_probe_release'}
    check(symbols == expected, (symbols, expected))
    for forbidden in ['dcmtk', 'gdcm', 'pydicom', 'jpeg', 'jxl', 'charls', 'python', 'jni']:
        check(forbidden not in dependencies.lower(), forbidden)
    print(json.dumps({'kind': 'binary_inspection', 'symbols': sorted(symbols), 'file': call(['file', library]).strip(), 'dependencies': dependencies, 'outcome': 'passed', 'python_optimize': sys.flags.optimize}))
if __name__ == '__main__':
    main()
