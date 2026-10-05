"""Document/specification checks, explicitly not DICOM engine acceptance."""

def check(condition, message):
    if not condition:
        raise RuntimeError(str(message))
import hashlib
import json
import sys
import argparse
import re
from pathlib import Path

def validate_type2_prescriptions(profile):
    count = 0
    for field in profile['attributes']:
        if field['presence'] != 'present' or field['type'] not in ('2', '2C'):
            continue
        count += 1
        tests = field['tests']
        label = field['id'] + ': Type 2 acceptance'
        check(field['condition_true'] and not field['depth'], label + ' condition')
        check(tests.get('element_presence_required') is True, label + ' missing-element rule')
        check(tests.get('empty_value_allowed') is True, label + ' legal empty value')
        check(tests.get('fixture_equality_scope') == 'fixture_only', label + ' fixture scope')
        negative = tests['negative_expectation'].lower()
        check(not re.search(r'(reject|fail)[^.;]*\bempty\b', negative.replace('non-empty', 'populated')),
              label + ' rejects empty values')
        check('missing element' in negative and 'malformed non-empty' in negative,
              label + ' missing/malformed distinction')
        check('fixture equality only' in negative, label + ' conflates fixture and semantic validity')
    check(count == 15, 'Expected all 15 present Type 2/2C prescriptions')
    for tag, value in [('(0040,0555)', []), ('(0020,0020)', '')]:
        fields = [f for f in profile['attributes'] if f['tag'] == tag and not f['depth']]
        check(bool(fields), 'Missing positive empty witness ' + tag)
        for field in fields:
            check(field['presence'] == 'present' and field['fixture_value'] == value,
                  'Empty fixed fixture must be present: ' + tag)
            check(field['tests'].get('empty_value_allowed') is True,
                  'Empty positive witness rejected: ' + tag)
    return count

def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument('--inventory', type=Path, default=repo / 'docs/profiles/DF-PHOTO-RGB8-v0.1.0.json')
    p = json.loads(parser.parse_args().inventory.read_text())
    validate_type2_prescriptions(p)
    expanded = [m['reference'] for m in p['macro_invocations'] if m['disposition'] == 'expanded']
    check((len(p['coverage']['expanded_tables']), len(expanded), len(set(expanded))) == (19, 10, 9),
          'Expanded table/invocation/distinct macro counts')
    required = {'Patient', 'General Study', 'General Series', 'General Equipment', 'General Acquisition', 'General Image', 'Image Pixel', 'Acquisition Context', 'VL Image', 'SOP Common'}
    check({m['module'] for m in p['modules'] if m['included']} == required, "{m['module'] for m in p['modules'] if m['included']} == required")
    ids = set()
    selected = {}
    for field in p['attributes']:
        check(field['id'] not in ids, "field['id'] not in ids")
        ids.add(field['id'])
        for key in ['tag', 'vr', 'vm', 'type', 'condition', 'value_source', 'encoding_rule', 'empty_absent_behavior', 'source']:
            check(field[key], (field['id'], key))
        check(field['tests']['engine_execution'] == 'not_run', "field['tests']['engine_execution'] == 'not_run'")
        if field['depth']:
            check(field['presence'] == 'absent', "field['presence'] == 'absent'")
        if field['presence'] == 'present':
            check(field['condition_true'], "field['condition_true']")
            if field['type'].startswith('1'):
                check(field['fixture_value'] not in ['', [], None], "field['fixture_value'] not in ['', [], None]")
            if field['tag'] in selected:
                check(selected[field['tag']] == field['fixture_value'], "selected[field['tag']] == field['fixture_value']")
            selected[field['tag']] = field['fixture_value']
        elif field['type'].endswith('C'):
            check(not field['condition_true'], "not field['condition_true']")
    check(selected['(0040,0555)'] == [], "selected['(0040,0555)'] == []")
    check(selected['(0020,0020)'] == '', "selected['(0020,0020)'] == ''")
    check(selected['(0028,0002)'] == 3 and selected['(0028,0006)'] == 0, "selected['(0028,0002)'] == 3 and selected['(0028,0006)'] == 0")
    check(selected['(0028,0100)'] == 8 and selected['(0028,0103)'] == 0, "selected['(0028,0100)'] == 8 and selected['(0028,0103)'] == 0")
    for m in p['macro_invocations']:
        check(m['reference'] and m['condition'], "m['reference'] and m['condition']")
    fixtures = repo / 'tests/fixtures/synthetic'
    manifest = json.loads((fixtures / 'pixel-manifest.json').read_text())
    for record in manifest['fixtures']:
        data = (fixtures / record['file']).read_bytes()
        check(hashlib.sha256(data).hexdigest() == record['sha256'], "hashlib.sha256(data).hexdigest() == record['sha256']")
        check(len(data) == record['rows'] * record['columns'] * 3 == record['logical_bytes'], "len(data) == record['rows'] * record['columns'] * 3 == record['logical_bytes']")
    large = (fixtures / 'rgb16x16.rgb').read_bytes()
    check(large[:6] == bytes.fromhex('000000110011'), "large[:6] == bytes.fromhex('000000110011')")
    check(large[48:51] == bytes.fromhex('001111'), "large[48:51] == bytes.fromhex('001111')")
    check(large[-3:] == bytes.fromhex('ffff00'), "large[-3:] == bytes.fromhex('ffff00')")
    check((fixtures / 'rgb1x3-odd.rgb').read_bytes() == bytes.fromhex('000000110011220022'), "(fixtures / 'rgb1x3-odd.rgb').read_bytes() == bytes.fromhex('000000110011220022')")
    meta = json.loads((repo / 'docs/profiles/file-meta-inventory.json').read_text())
    check(len(meta['attributes']) == 15, "len(meta['attributes']) == 15")
    check(len({f['tag'] for f in meta['attributes']}) == 15, "len({f['tag'] for f in meta['attributes']}) == 15")
    check({f['vr'] for f in meta['attributes'] if f['presence'] == 'present'} <= set(p['required_df1_vrs']), "{f['vr'] for f in meta['attributes'] if f['presence'] == 'present'} <= set(p['required_df1_vrs'])")
    golden = json.loads((fixtures / 'golden-layout.json').read_text())
    selected_meta = {f['tag'].replace('(', '').replace(')', '').replace(',', ''): f for f in meta['attributes'] if f['presence'] == 'present'}
    selected_dataset = {tag.replace('(', '').replace(')', '').replace(',', ''): value for (tag, value) in selected.items()}
    check(golden['filemeta_group_length'] == selected_meta['00020000']['fixture_value'] == 204, "golden['filemeta_group_length'] == selected_meta['00020000']['fixture_value'] == 204")
    check(golden['dataset_offset'] == 348, "golden['dataset_offset'] == 348")
    check(golden['rgb16x16']['object_bytes'] == 1638, "golden['rgb16x16']['object_bytes'] == 1638")
    check(golden['rgb1x3_odd']['object_bytes'] == 880, "golden['rgb1x3_odd']['object_bytes'] == 880")
    check({f['tag'] for f in golden['rgb16x16']['elements']} == set(selected_meta) | set(selected_dataset), "{f['tag'] for f in golden['rgb16x16']['elements']} == set(selected_meta) | set(selected_dataset)")
    for variant in ['rgb16x16', 'rgb1x3_odd']:
        previous = 132
        for field in golden[variant]['elements']:
            check(field['header_offset'] == previous, "field['header_offset'] == previous")
            check(field['encoded_value_length'] % 2 == 0, "field['encoded_value_length'] % 2 == 0")
            check(field['end_offset'] == field['value_offset'] + field['encoded_value_length'], "field['end_offset'] == field['value_offset'] + field['encoded_value_length']")
            previous = field['end_offset']
        check(previous == golden[variant]['object_bytes'], "previous == golden[variant]['object_bytes']")
    print(json.dumps({'kind': 'document_and_owned_fixture_check', 'outcome': 'passed', 'python_optimize': sys.flags.optimize, 'mandatory_modules': len(required), 'attribute_rows': len(ids), 'selected_tags': len(selected), 'file_meta_attribute_rows': 15, 'expected_layouts': 2, 'engine_profile_validation': 'not_run'}))
if __name__ == '__main__':
    main()
