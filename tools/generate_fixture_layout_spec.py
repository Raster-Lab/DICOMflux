"""Independent specification arithmetic, not a DICOM serializer."""
from pathlib import Path
import json
repo=Path(__file__).resolve().parents[1]
uid=lambda end:'2.25.302661813519380590220360801495109412'+end
sop='1.2.840.10008.5.1.4.1.1.77.1.4'
# Written independently of inventory extraction and future native code.
dataset=[('00080008','CS','ORIGINAL\\PRIMARY'),('00080016','UI',sop),('00080018','UI',uid('004')),
 ('00080020','DA','20261004'),('00080030','TM','120000'),('00080050','SH',''),('00080060','CS','XC'),
 ('00080070','LO','DICOMflux Synthetic'),('00080090','PN',''),('00100010','PN','SYNTHETIC^DF0'),
 ('00100020','LO','DF0-TEST'),('00100030','DA',''),('00100040','CS',''),('0020000D','UI',uid('001')),
 ('0020000E','UI',uid('002')),('00200010','SH','DF0'),('00200011','IS','1'),('00200013','IS','1'),
 ('00200020','CS',''),('00280002','US',3),('00280004','CS','RGB'),('00280006','US',0),
 ('00280010','US',16),('00280011','US',16),('00280100','US',8),('00280101','US',8),
 ('00280102','US',7),('00280103','US',0),('00282110','CS','00'),('00400555','SQ',[])]
filemeta=[('00020000','UL',0),('00020001','OB',2),('00020002','UI',sop),('00020003','UI',uid('004')),
          ('00020010','UI','1.2.840.10008.1.2.1'),('00020012','UI',uid('900')),('00020013','SH','DF_SPEC_0001')]
def length(vr,value):
    if vr=='US':return 2
    if vr=='UL':return 4
    if vr=='SQ':return 0
    if vr=='OB':return value
    return len(value.encode('ascii'))
def layout(rows):
    offset=132;items=[]
    for tag,vr,value in rows:
        logical=length(vr,value);encoded=logical+(logical%2);header=12 if vr in ['OB','SQ'] else 8
        items.append({'tag':tag,'vr':vr,'header_offset':offset,'value_offset':offset+header,
                      'logical_length':logical,'encoded_value_length':encoded,'end_offset':offset+header+encoded})
        offset+=header+encoded
    return items,offset
even,total=layout(filemeta+dataset+[('7FE00010','OB',768)])
odd_dataset=[(tag,vr,1 if tag=='00280010' else 3 if tag=='00280011' else uid('005') if tag=='00080018' else value)
             for tag,vr,value in dataset]
odd_meta=[(tag,vr,uid('005') if tag=='00020003' else value) for tag,vr,value in filemeta]
odd,odd_total=layout(odd_meta+odd_dataset+[('7FE00010','OB',9)])
dataset_start=next(e['header_offset'] for e in even if e['tag']=='00080008')
selected=[]
for tag in ['00020000','00020001','00200020','00400555','7FE00010']:
    for e in even:
        if e['tag']==tag:selected.append(e)
golden={'status':'independently authored specification; no DICOM file generated or parsed',
 'profile':'DF-PHOTO-RGB8-v0.1.0','preamble':{'offset':0,'length':128,'byte':0},
 'prefix':{'offset':128,'hex':'4449434d'},'filemeta_group_length':dataset_start-144,
 'filemeta_version_value':{'offset':156,'hex':'0001'},'dataset_offset':dataset_start,
 'empty_acquisition_context_header_hex':'400055055351000000000000',
 'patient_orientation_header_hex':'2000200043530000',
 'rgb16x16':{'object_bytes':total,'elements':even},
 'rgb1x3_odd':{'object_bytes':odd_total,'elements':odd,'pad_byte':0,'pad_offset':odd_total-1},
 'notes':['File Meta and dataset SOP identities must match.','The odd fixture uses a distinct test instance UID.',
          'Header offsets are a specification witness only; native writer and independent-oracle checks belong to DF-1.']}
(repo/'tests/fixtures/synthetic/golden-layout.json').write_text(json.dumps(golden,indent=2)+'\n')
print(json.dumps({'even_total':total,'odd_total':odd_total,'dataset_offset':dataset_start,'group_length':dataset_start-144,'selected':selected},indent=2))
