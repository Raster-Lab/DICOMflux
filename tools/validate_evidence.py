"""Check DF-0 record consistency and tested-source identity, not engine acceptance."""

def check(condition, message):
    if not condition:
        raise RuntimeError(str(message))
import hashlib
import json
import sys
import argparse
import subprocess
from pathlib import Path

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--input-manifest', type=Path)
    args = parser.parse_args()
    repo = args.repo.resolve()
    evidence = repo / 'docs/evidence'
    schema = json.loads((evidence / 'test-result.schema.json').read_text())
    count = failures = 0
    for path in (evidence / 'records').rglob('*.json'):
        record = json.loads(path.read_text())
        if not isinstance(record, dict) or 'exit_code' not in record:
            continue
        check(set(schema['required']) <= record.keys(), path)
        check(record['schema_version'] == '1', path)
        check(record['kind'] in schema['properties']['kind']['enum'], path)
        check(isinstance(record['exit_code'], int), path)
        check(record['elapsed_seconds'] >= 0, path)
        check(record['command'] and all((isinstance(s, str) for s in record['command'])), path)
        check(all((isinstance(record[k], str) for k in ['id', 'cwd', 'started_at', 'stdout', 'stderr', 'scope'])), path)
        check(record['outcome'] == ('passed' if record['exit_code'] == 0 else 'failed'), path)
        check('/Users/' not in path.read_text(), path)
        count += 1
        failures += record['exit_code'] != 0
    source = json.loads((evidence / 'build-source-files.json').read_text())
    # Historical hashes belong to the provisional checkpoint, not edited files.
    checkpoint = '59d465f49f3caaa9a7e89b28f92222b4752d068a'
    for (name, digest) in source.items():
        data = subprocess.check_output(['git', 'show', checkpoint + ':' + name], cwd=repo)
        check(hashlib.sha256(data).hexdigest() == digest, name)
    if args.input_manifest:
        for name, digest in json.loads(args.input_manifest.read_text()).items():
            check(hashlib.sha256((repo / name).read_bytes()).hexdigest() == digest, name)
    acceptance = json.loads((evidence / 'engine-acceptance-status.json').read_text())
    check(len(acceptance) == 24, 'len(acceptance) == 24')
    check(all((r['engine_verification_status'] == 'not_run' for r in acceptance)), "all((r['engine_verification_status'] == 'not_run' for r in acceptance))")
    gate = json.loads((evidence / 'df-g0-review-request.json').read_text())
    check(gate['human_disposition'] is None and (not gate['df1_authorised']), "gate['human_disposition'] is None and (not gate['df1_authorised'])")
    print(json.dumps({'kind': 'document_check', 'outcome': 'passed', 'python_optimize': sys.flags.optimize, 'command_records': count, 'preserved_failed_records': failures, 'historical_source_files_verified_at_checkpoint': len(source), 'historical_checkpoint': checkpoint, 'current_input_manifest_checked': args.input_manifest is not None, 'engine_tests_not_run': 24, 'gate': 'DF-G0 pending human review', 'validation': 'required fields, types, status consistency and source hashes; not a general JSON Schema validator; no retroactive linkage for unbound runs'}))
if __name__ == '__main__':
    main()
