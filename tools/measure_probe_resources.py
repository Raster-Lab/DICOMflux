#!/usr/bin/env python3
"""Measure one native prototype process; RSS is distinct from tracked bytes."""
import argparse
import json
import platform
import resource
import subprocess
import time
from pathlib import Path

def main():
    parser=argparse.ArgumentParser();parser.add_argument('probe',type=Path);args=parser.parse_args()
    start=time.monotonic()
    result=subprocess.run([str(args.probe.resolve())],text=True,capture_output=True)
    usage=resource.getrusage(resource.RUSAGE_CHILDREN)
    factor=1 if platform.system()=='Darwin' else 1024
    print(json.dumps({'kind':'resource_measurement','scope':'one foundation-test process; not a writer benchmark',
      'command':[str(args.probe.resolve())],'exit_code':result.returncode,'elapsed_seconds':time.monotonic()-start,
      'peak_process_rss_bytes':usage.ru_maxrss*factor,'user_seconds':usage.ru_utime,'system_seconds':usage.ru_stime,
      'stdout':result.stdout,'stderr':result.stderr,'limitations':'RSS includes runtime/test process; library tracked peak is reported separately by the probe. Single run, cache state uncontrolled; no throughput comparison.'}))
    return result.returncode

if __name__=='__main__':raise SystemExit(main())
