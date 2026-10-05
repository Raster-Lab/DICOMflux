#!/usr/bin/env python3
"""Check DF-0 record consistency and tested-source identity, not engine acceptance."""
import hashlib
import json
from pathlib import Path


def main():
    repo = Path(__file__).resolve().parents[1]
    evidence = repo / 'docs/evidence'
    schema = json.loads((evidence / 'test-result.schema.json').read_text())
    count = failures = 0
    for path in (evidence / 'records').rglob('*.json'):
        record = json.loads(path.read_text())
        if not isinstance(record, dict) or 'exit_code' not in record:
            continue
        assert set(schema['required']) <= record.keys(), path
        assert record['schema_version'] == '1', path
        assert record['kind'] in schema['properties']['kind']['enum'], path
        assert isinstance(record['exit_code'], int), path
        assert record['elapsed_seconds'] >= 0, path
        assert record['command'] and all(isinstance(s, str) for s in record['command']), path
        assert all(isinstance(record[k], str) for k in ['id', 'cwd', 'started_at', 'stdout', 'stderr', 'scope']), path
        assert record['outcome'] == ('passed' if record['exit_code'] == 0 else 'failed'), path
        assert '/Users/' not in path.read_text(), path
        count += 1
        failures += record['exit_code'] != 0
    source = json.loads((evidence / 'build-source-files.json').read_text())
    for name, digest in source.items():
        assert hashlib.sha256((repo / name).read_bytes()).hexdigest() == digest, name
    acceptance = json.loads((evidence / 'engine-acceptance-status.json').read_text())
    assert len(acceptance) == 24
    assert all(r['engine_verification_status'] == 'not_run' for r in acceptance)
    gate = json.loads((evidence / 'df-g0-review-request.json').read_text())
    assert gate['human_disposition'] is None and not gate['df1_authorised']
    print(json.dumps({'kind': 'document_check', 'outcome': 'passed',
                      'command_records': count, 'preserved_failed_records': failures,
                      'tested_source_files_unchanged': len(source),
                      'engine_tests_not_run': 24, 'gate': 'DF-G0 pending human review',
                      'validation': 'required fields, types, status consistency and source hashes; not a general JSON Schema validator'}))


if __name__ == '__main__':
    main()
