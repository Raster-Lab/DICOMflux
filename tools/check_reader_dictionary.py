"""Isolated pydicom setup/selected-tag check; never engine or IOD acceptance.

Run with python -I -S and a separately acquired, hash-pinned wheel. The wheel is
unpacked into a new private directory, never installed in the project/runtime.
"""

import argparse
import hashlib
import io
import json
from pathlib import Path
import sys
import zipfile


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--wheel', required=True, type=Path)
    parser.add_argument('--scratch', required=True, type=Path,
                        help='New directory outside the checkout; retained for review')
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    require(sys.flags.isolated and sys.flags.no_site, 'Use python -I -S for isolation')
    repo = args.repo.resolve()
    manifest = json.loads((repo / 'third_party/manifest/oracles.json').read_text())
    pin = next(row for row in manifest['oracles'] if row['name'] == 'pydicom')
    wheel = args.wheel.resolve()
    digest = hashlib.sha256(wheel.read_bytes()).hexdigest()
    require(digest == pin['sha256'], 'Wheel hash mismatch; refusing import')
    require('pydicom' not in sys.modules, 'pydicom was imported before hash validation')
    scratch = args.scratch.resolve()
    require(scratch != repo and repo not in scratch.parents, 'Scratch must be outside Git')
    scratch.mkdir(parents=True, exist_ok=False)
    with zipfile.ZipFile(wheel) as archive:
        for name in archive.namelist():
            target = (scratch / name).resolve()
            require(scratch in target.parents, 'Unsafe wheel member path')
        archive.extractall(scratch)
    def offline(event, args):
        if event in ('socket.connect', 'socket.getaddrinfo'):
            raise RuntimeError('Oracle setup does not permit network access')
    sys.addaudithook(offline)
    sys.dont_write_bytecode = True
    sys.path.insert(0, str(scratch))
    import pydicom
    from pydicom.datadict import dictionary_VM, dictionary_VR
    from pydicom.errors import InvalidDicomError
    require(pydicom.__file__.startswith(str(scratch) + '/'), 'Wrong pydicom origin')
    require(pydicom.__version__ == pin['version'], 'Wrong pydicom version')

    selected = {}
    input_hashes = {}
    for name in ('docs/profiles/DF-PHOTO-RGB8-v0.1.0.json',
                 'docs/profiles/file-meta-inventory.json'):
        data = (repo / name).read_bytes()
        input_hashes[name] = hashlib.sha256(data).hexdigest()
        for row in json.loads(data)['attributes']:
            if row['presence'] == 'present':
                tag = row['tag']
                definition = (row['vr'], row['vm'])
                require(tag not in selected or selected[tag] == definition,
                        'Inconsistent duplicate selected tag: ' + tag)
                selected[tag] = definition

    rows = []
    for tag, (vr, vm) in sorted(selected.items()):
        number = int(tag.replace('(', '').replace(')', '').replace(',', ''), 16)
        try:
            oracle_vr, oracle_vm = dictionary_VR(number), dictionary_VM(number)
        except KeyError:
            oracle_vr = oracle_vm = None
        rows.append({'tag': tag, 'profile_vr': vr, 'profile_vm': vm,
                     'oracle_vr': oracle_vr, 'oracle_vm': oracle_vm,
                     'matches': oracle_vr == vr and oracle_vm == vm})
    rejected = False
    try:
        pydicom.dcmread(io.BytesIO(b'DF0-owned-invalid-not-a-Part10-file'), force=False)
    except InvalidDicomError:
        rejected = True
    result = {
        'kind': 'external_oracle_setup_and_dictionary_check',
        'version': pydicom.__version__, 'wheel_sha256': digest,
        'python_optimize': sys.flags.optimize, 'profile_inputs': input_hashes,
        'selected_unique_tags': len(rows), 'matching_tags': sum(r['matches'] for r in rows),
        'strict_non_part10_rejected': rejected, 'tags': rows,
        'scope': 'Selected fixture tag VR/VM lookup and invalid-input rejection only',
        'positive_part10_read': 'not_run', 'engine_object_validation': 'not_run',
        'iod_validation': 'not_provided_by_this_reader',
        'excluded_condition_coverage': 'not_evaluated',
    }
    print(json.dumps(result, indent=2))
    require(len(rows) == 38, 'Expected 31 dataset and 7 file-meta unique selected tags')
    require(all(row['matches'] for row in rows), 'Selected-tag dictionary mismatch')
    require(rejected, 'Strict reader accepted invalid non-Part10 input')


if __name__ == '__main__':
    main()
