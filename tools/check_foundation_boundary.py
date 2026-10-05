"""Inspect the actual host library/installed metadata; no platform qualification inference."""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import platform
import re
import subprocess
import sys

FUNCTIONS = {'dicomflux_query_abi', 'dicomflux_context_create', 'dicomflux_context_retain',
             'dicomflux_context_release', 'dicomflux_error_copy'}

def require(condition, message):
    if not condition:
        raise RuntimeError(message)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', type=Path, required=True)
    parser.add_argument('--capabilities', type=Path, required=True)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    repo = args.repo.resolve()
    require(platform.system() == 'Darwin', 'This inspection witness is macOS-only; other targets remain unqualified')
    symbols = subprocess.check_output(['/usr/bin/nm', '-gU', str(args.library)], text=True)
    exported = {line.split()[-1].removeprefix('_') for line in symbols.splitlines() if line.split()}
    require(exported == FUNCTIONS, 'Unexpected dynamic export surface: ' + repr(sorted(exported)))
    dependencies = subprocess.check_output(['/usr/bin/otool', '-L', str(args.library)], text=True)
    lines = [line.strip().split(' (', 1)[0] for line in dependencies.splitlines()[1:]]
    require(len(lines) == 3, 'Unexpected dependency count')
    require(lines[0].endswith('/libdicomflux_foundation.dylib'), 'Unexpected library identity')
    require(set(lines[1:]) == {'/usr/lib/libc++.1.dylib', '/usr/lib/libSystem.B.dylib'}, 'Unexpected dependency/backend')
    capabilities = json.loads(args.capabilities.read_text())
    require(capabilities['authority'] == 'DF-A002', 'Wrong capability authority')
    require(set(capabilities['functions']) == FUNCTIONS and len(capabilities['functions']) == 5, 'Capability surface mismatch')
    require(capabilities['experimental_abi'] == {'major': 0, 'minor': 1, 'stable_release': False}, 'ABI identity mismatch')
    require(not capabilities['dicom_read_write'] and not capabilities['networking'] and not capabilities['public_cancellation'], 'Excluded capability claimed')
    require(not capabilities['sop_profiles'] and not capabilities['codecs'] and not capabilities['third_party_dicom_backends'] and not capabilities['transfer_syntaxes_by_direction'], 'Excluded backend/profile claimed')
    require(set(capabilities['source_sha256']) == {'include/dicomflux/foundation.h', 'src/foundation/core.hpp',
            'src/foundation/core.cpp', 'src/c_api/foundation.cpp'}, 'Incomplete source identity')
    for name, expected in capabilities['source_sha256'].items():
        require(hashlib.sha256((repo / name).read_bytes()).hexdigest() == expected, 'Build source changed: ' + name)
    header = (repo / 'include/dicomflux/foundation.h').read_text()
    declarations = set(re.findall(r'DICOMFLUX_API\s+(?:dicomflux_status|void)\s+(dicomflux_\w+)\s*\(', header))
    require(declarations == FUNCTIONS, 'Installed declaration allowlist mismatch')
    authority = json.loads((repo / 'docs/evidence/effective-authority.json').read_text())
    require(authority['authority_id'] == 'DF-A002' and authority['df_g0_disposition'] == 'accepted_narrow_auxiliary_host_foundation', 'Wrong effective authority')
    execution = authority['execution_authority']
    require(not execution['full_df1_authorised'] and set(execution['public_function_allowlist']) == FUNCTIONS, 'Scope widened')
    require(not any(isinstance(node, ast.Assert) for node in ast.walk(ast.parse(Path(__file__).read_text()))), 'Removable acceptance check')
    print(json.dumps({'kind': 'actual_binary_and_scope_inspection', 'python_optimize': sys.flags.optimize,
                      'functions': sorted(exported), 'dependencies': lines, 'library_sha256': hashlib.sha256(args.library.read_bytes()).hexdigest(),
                      'source_sha256': capabilities['source_sha256'], 'outcome': 'passed',
                      'scope': 'DF-A002 auxiliary host foundation; no consumer or writer qualification'}))

if __name__ == '__main__':
    main()
