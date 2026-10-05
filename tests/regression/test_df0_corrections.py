"""DF-0 document/harness regressions; no writer or target qualification."""
import argparse
import ast
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

REPO = Path(__file__).resolve().parents[2]
PROFILE = REPO / 'docs/profiles/DF-PHOTO-RGB8-v0.1.0.json'
LIBRARY = None


def load(name):
    spec = importlib.util.spec_from_file_location(name, REPO / 'tools' / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class Corrections(unittest.TestCase):
    def setUp(self):
        root = REPO / 'build/df0-regression-inputs'
        root.mkdir(parents=True, exist_ok=True)
        # Retain actual mutation bytes for the parent evidence runner to archive.
        self.work = Path(tempfile.mkdtemp(dir=root))
        self.profile = json.loads(PROFILE.read_text())

    def write(self, name, value):
        path = self.work / name
        if path.exists():
            raise RuntimeError('Refusing to overwrite a retained mutation input')
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(value, indent=2) + '\n')
        print('mutation-input', name, hashlib.sha256(path.read_bytes()).hexdigest(), flush=True)
        return path

    def run_cli(self, script, args, optimized, expected):
        command = [sys.executable] + (['-O'] if optimized else []) + [str(REPO / script), *map(str, args)]
        env = os.environ.copy()
        env.pop('PYTHONOPTIMIZE', None)
        result = subprocess.run(command, text=True, capture_output=True, cwd=REPO, env=env)
        record = {'command': command, 'exit_code': result.returncode, 'stdout': result.stdout,
                  'stderr': result.stderr, 'expected_exit_code': expected}
        path = self.work / ('command-%02d.json' % len(list(self.work.glob('command-*.json'))))
        path.write_text(json.dumps(record, indent=2) + '\n')
        print(script, 'optimize=', optimized, 'exit=', result.returncode, 'expected=', expected, flush=True)
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        if expected:
            self.assertNotIn('"passed"', result.stdout)
        return result

    def test_all_type2_contradictions(self):
        validator = load('validate_profile_inventory')
        self.assertEqual(validator.validate_type2_prescriptions(self.profile), 15)
        indices = [i for i, f in enumerate(self.profile['attributes'])
                   if f['presence'] == 'present' and f['type'] in ('2', '2C')]
        for i in indices:
            bad = copy.deepcopy(self.profile)
            bad['attributes'][i]['tests']['negative_expectation'] = 'reject missing/empty/mismatched required value'
            with self.subTest(field=bad['attributes'][i]['id']):
                with self.assertRaisesRegex(RuntimeError, 'rejects empty'):
                    validator.validate_type2_prescriptions(bad)
        for key, value in [('empty_value_allowed', False), ('element_presence_required', False),
                           ('fixture_equality_scope', 'all_values')]:
            bad = copy.deepcopy(self.profile)
            bad['attributes'][indices[0]]['tests'][key] = value
            with self.assertRaises(RuntimeError):
                validator.validate_type2_prescriptions(bad)

    def test_empty_positive_witnesses(self):
        validator = load('validate_profile_inventory')
        for tag in ['(0040,0555)', '(0020,0020)']:
            bad = copy.deepcopy(self.profile)
            row = next(f for f in bad['attributes'] if f['tag'] == tag and not f['depth'])
            row['fixture_value'] = 'unexpected nonempty value'
            with self.assertRaisesRegex(RuntimeError, 'Empty fixed fixture'):
                validator.validate_type2_prescriptions(bad)
            row['fixture_value'] = [] if tag == '(0040,0555)' else ''
            row['presence'] = 'absent'
            with self.assertRaises(RuntimeError):
                validator.validate_type2_prescriptions(bad)

    def test_profile_process_modes(self):
        for optimized in [False, True]:
            self.run_cli('tools/validate_profile_inventory.py', [], optimized, 0)
            for tag in ['(0040,0555)', '(0020,0020)']:
                bad = copy.deepcopy(self.profile)
                row = next(f for f in bad['attributes'] if f['tag'] == tag and not f['depth'])
                row['tests']['negative_expectation'] = 'reject missing/empty/mismatched required value'
                suffix = tag.strip('()').replace(',', '-')
                path = self.write('bad-profile-' + suffix + '-' + str(int(optimized)) + '.json', bad)
                self.run_cli('tools/validate_profile_inventory.py', ['--inventory', path], optimized, 1)

    def test_native_probe_process_modes(self):
        self.assertIsNotNone(LIBRARY, '--library is required')
        budgets = REPO / 'docs/architecture/qualification-budgets.json'
        bad = json.loads(budgets.read_text())
        bad['transfer_chunk_bytes'] = 0  # Native create must reject this, not vanish under -O.
        invalid = self.write('invalid-budgets.json', bad)
        for optimized in [False, True]:
            result = self.run_cli('bindings/python_probe/probe.py', [LIBRARY, '--budgets', budgets], optimized, 0)
            report = json.loads(result.stdout)
            self.assertGreater(report['abi_options_size'], 0)
            self.assertGreater(report['abi_options_alignment'], 0)
            self.assertEqual(report['python_optimize'], int(optimized))
            result = self.run_cli('bindings/python_probe/probe.py', [LIBRARY, '--budgets', invalid], optimized, 1)
            self.assertIn('dicomflux_probe_create', result.stderr)

    def test_evidence_false_pass_rejected(self):
        evidence = self.work / 'docs/evidence'
        evidence.mkdir(parents=True)
        (evidence / 'test-result.schema.json').write_bytes((REPO / 'docs/evidence/test-result.schema.json').read_bytes())
        record = json.loads((REPO / 'docs/evidence/records/final/host-unit.json').read_text())
        record['outcome'] = 'passed'
        record['exit_code'] = 1
        self.write('docs/evidence/records/bad.json', record)
        for optimized in [False, True]:
            self.run_cli('tools/validate_evidence.py', ['--repo', self.work], optimized, 1)

    def test_binary_policy_rejects_extra_export_and_dependency(self):
        checker = load('check_probe_binary')
        symbols = '\n'.join('0000 T _' + n for n in ['dicomflux_probe_query_abi',
            'dicomflux_probe_create', 'dicomflux_probe_measure', 'dicomflux_probe_release'])
        for exports, dependencies in [(symbols + '\n0000 T _unexpected', 'libSystem'),
                                      (symbols, 'libdcmtk')]:
            with patch.object(sys, 'argv', ['check_probe_binary.py', 'unused']), \
                 patch.object(checker.platform, 'system', return_value='Darwin'), \
                 patch.object(checker.subprocess, 'check_output', side_effect=[exports, dependencies]):
                with self.assertRaises(RuntimeError):
                    checker.main()

    def test_no_removable_python_acceptance_checks(self):
        paths = list((REPO / 'tools').glob('*.py')) + list((REPO / 'bindings/python_probe').glob('*.py'))
        for path in paths:
            self.assertFalse(any(isinstance(n, ast.Assert) for n in ast.walk(ast.parse(path.read_text()))), str(path))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--library', type=Path, required=True)
    args, remaining = parser.parse_known_args()
    LIBRARY = args.library.resolve()
    unittest.main(argv=[sys.argv[0], *remaining], verbosity=2)
