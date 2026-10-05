#!/usr/bin/env python3
"""Owned source pixels only. Does not generate a DICOM file or call an oracle."""
import hashlib
import json
from pathlib import Path

def pixels(rows,columns):
    return bytes(channel for y in range(rows) for x in range(columns)
                 for channel in (17*x,17*y,17*(x^y)))

def main():
    output=Path(__file__).resolve().parents[1]/'tests/fixtures/synthetic'
    output.mkdir(parents=True,exist_ok=True)
    records=[]
    for name,rows,columns in [('rgb16x16',16,16),('rgb1x3-odd',1,3)]:
        data=pixels(rows,columns)
        path=output/(name+'.rgb');path.write_bytes(data)
        records.append({'name':name,'file':path.name,'rows':rows,'columns':columns,
                        'samples_per_pixel':3,'logical_bytes':len(data),
                        'encoded_pixel_value_bytes':len(data)+(len(data)%2),
                        'padding_byte':0 if len(data)%2 else None,
                        'sha256':hashlib.sha256(data).hexdigest()})
    manifest={'schema_version':'1','kind':'owned_synthetic_raw_pixels_not_DICOM',
              'formula':'r=17*x, g=17*y, b=17*(x XOR y), row-major interleaved RGB',
              'licence':'MIT; Copyright (c) 2026 Raster Images','fixtures':records}
    (output/'pixel-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(manifest,indent=2))

if __name__=='__main__':main()
