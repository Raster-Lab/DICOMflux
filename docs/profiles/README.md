# Photographic fixture specification for DF-G0

The JSON inventory covers all 10 mandatory modules in the VL Photographic Image
IOD, 329 attribute rows, 19 expanded inherited macro tables, and explicit
conditions for the narrow non-specimen, non-stereo, single-frame human profile.
Optional modules are listed with their omission reason. Nested item macros
under absent/empty sequences are recorded as uninstantiated; non-empty sequences
are outside this candidate. Every row carries planned positive/negative checks.
These are specification checks; no DICOM engine profile test has executed.

The selected dataset contains 31 unique tags. File Meta has a separate complete
15-attribute inventory plus preamble/prefix. The required DF-1 VR set is CS, DA,
IS, LO, OB, PN, SH, SQ, TM, UI, UL and US. UL is needed for File Meta group length;
SQ supports only the mandatory empty Acquisition Context Sequence in this slice.
Type 2C Patient Orientation is present with zero length; optional anatomy and
Content Date/Time are absent under the recorded single-image conditions.

`tests/fixtures/synthetic/` contains owned raw RGB bytes, not DICOM objects. The
16×16 formula is R=17x, G=17y, B=17(x XOR y), row-major interleaved. The odd fixture
is 1×3 (9 logical bytes); its expected encoded Pixel Data length is 10. Both use
fixed test-only UIDs and deterministic metadata. Never reuse these UIDs for
real patient objects or silently generate timestamps/identities.

The independent `golden-layout.json` specifies 128 zero bytes, DICM at 128, File
Meta group length 204, dataset start 348 and Pixel Data value start 870. Expected
total lengths are 1638 and 880 bytes. Empty Acquisition Context has the 12-byte
header `400055055351000000000000`, with zero VL and no items/delimiters. An empty
Patient Orientation element has `2000200043530000`. The odd pad byte at offset
879 is zero and is outside the 9 logical samples. All offsets are zero-based.

`tools/generate_fixture_layout_spec.py` calculates the independent expected
layout; it does not serialize a DICOM file or invoke the prototype. The raw
pixel generator and layout specification are separate from future native code.
`tools/validate_profile_inventory.py` checks internal consistency, required
module coverage, fixture hashes, independent selected samples and byte spans.
A passing document check cannot prove IOD validity.

Normative references: [IOD modules](https://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_A.32.4.3.html),
[IOD constraints](https://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_A.32.4.4.html),
[File Meta](https://dicom.nema.org/medical/dicom/current/output/chtml/part10/chapter_7.html).
Exact local capture hashes and the limited rights boundary are in
`third_party/manifest/standards.json`. These mutable official pages were captured
as observed edition 2026d; a verified immutable upstream release remains open.
