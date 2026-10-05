# Synthetic source fixtures

These are raw pixel sources for the DF-G0 fixture plan, not generated DICOM
objects. Run `python3 tools/generate_synthetic_pixels.py` from the checkout.
No patient records, camera output or external toolkit fixtures are used.

The even case is 16 by 16 pixels, 768 logical bytes. The odd case is 1 by 3,
9 logical bytes, requiring a tenth encoded byte of zero outside the samples.
The generator and the independent expectations use no writer implementation.
Fixed `2.25` identifiers in the profile specification are test-only; they must
never be reused for production instances or a production implementation identity.

Expected raw samples written independently of the generator:

| Location | R, G, B |
| --- | --- |
| (0,0) | 0, 0, 0 |
| (1,0) | 17, 0, 17 |
| (0,1) | 0, 17, 17 |
| (15,15) | 255, 255, 0 |
| Entire odd source | `00 00 00 11 00 11 22 00 22` |

The future Part 10 writer will be checked against `golden-layout.json`, exact
sample bytes, two independent readers and an IOD validator. No DICOM file or
validator outcome exists in DF-0. The strict-reader harness must never use a
force/permissive-read option, and warnings require a recorded disposition.
