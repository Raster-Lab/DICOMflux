#!/usr/bin/env python3
"""Document/specification checks, explicitly not DICOM engine acceptance."""
import hashlib
import json
from pathlib import Path

def main():
    repo=Path(__file__).resolve().parents[1]
    p=json.loads((repo/'docs/profiles/DF-PHOTO-RGB8-v0.1.0.json').read_text())
    required={'Patient','General Study','General Series','General Equipment','General Acquisition',
              'General Image','Image Pixel','Acquisition Context','VL Image','SOP Common'}
    assert {m['module'] for m in p['modules'] if m['included']}==required
    ids=set();selected={}
    for field in p['attributes']:
        assert field['id'] not in ids;ids.add(field['id'])
        for key in ['tag','vr','vm','type','condition','value_source','encoding_rule','empty_absent_behavior','source']:
            assert field[key],(field['id'],key)
        assert field['tests']['engine_execution']=='not_run'
        if field['depth']:assert field['presence']=='absent'
        if field['presence']=='present':
            assert field['condition_true']
            if field['type'].startswith('1'):assert field['fixture_value'] not in ['',[],None]
            if field['tag'] in selected:assert selected[field['tag']]==field['fixture_value']
            selected[field['tag']]=field['fixture_value']
        elif field['type'].endswith('C'):assert not field['condition_true']
    assert selected['(0040,0555)']==[]
    assert selected['(0020,0020)']==''
    assert selected['(0028,0002)']==3 and selected['(0028,0006)']==0
    assert selected['(0028,0100)']==8 and selected['(0028,0103)']==0
    for m in p['macro_invocations']:assert m['reference'] and m['condition']
    fixtures=repo/'tests/fixtures/synthetic'
    manifest=json.loads((fixtures/'pixel-manifest.json').read_text())
    for record in manifest['fixtures']:
        data=(fixtures/record['file']).read_bytes()
        assert hashlib.sha256(data).hexdigest()==record['sha256']
        assert len(data)==record['rows']*record['columns']*3==record['logical_bytes']
    large=(fixtures/'rgb16x16.rgb').read_bytes()
    assert large[:6]==bytes.fromhex('000000110011')
    assert large[48:51]==bytes.fromhex('001111')
    assert large[-3:]==bytes.fromhex('ffff00')
    assert (fixtures/'rgb1x3-odd.rgb').read_bytes()==bytes.fromhex('000000110011220022')
    meta=json.loads((repo/'docs/profiles/file-meta-inventory.json').read_text())
    assert len(meta['attributes'])==15
    assert len({f['tag'] for f in meta['attributes']})==15
    assert {f['vr'] for f in meta['attributes'] if f['presence']=='present'} <= set(p['required_df1_vrs'])
    golden=json.loads((fixtures/'golden-layout.json').read_text())
    selected_meta={f['tag'].replace('(','').replace(')','').replace(',',''):f for f in meta['attributes'] if f['presence']=='present'}
    selected_dataset={tag.replace('(','').replace(')','').replace(',',''):value for tag,value in selected.items()}
    assert golden['filemeta_group_length']==selected_meta['00020000']['fixture_value']==204
    assert golden['dataset_offset']==348
    assert golden['rgb16x16']['object_bytes']==1638
    assert golden['rgb1x3_odd']['object_bytes']==880
    assert {f['tag'] for f in golden['rgb16x16']['elements']}==set(selected_meta)|set(selected_dataset)
    for variant in ['rgb16x16','rgb1x3_odd']:
        previous=132
        for field in golden[variant]['elements']:
            assert field['header_offset']==previous
            assert field['encoded_value_length']%2==0
            assert field['end_offset']==field['value_offset']+field['encoded_value_length']
            previous=field['end_offset']
        assert previous==golden[variant]['object_bytes']
    print(json.dumps({'kind':'document_and_owned_fixture_check','outcome':'passed',
                      'mandatory_modules':len(required),'attribute_rows':len(ids),'selected_tags':len(selected),
                      'file_meta_attribute_rows':15,'expected_layouts':2,
                      'engine_profile_validation':'not_run'}))

if __name__=='__main__':main()
