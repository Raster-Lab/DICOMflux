# Independent oracle plan — unexecuted

All engine/interoperability tests remain not run. DF-0 creates no DICOM object.
The future fixture must originate in the owned native writer, never an oracle.
Pin tools using `third_party/manifest/oracles.json`; ordinary engine builds do
not acquire or link them. The DCMTK archive is currently blocked by a digest
discrepancy and must not be executed until reconciled.

After DF-1 is explicitly authorised and tools are qualified:

1. Capture exact `dcmdump --version` and `dciodvfy -version`, executable hashes,
   linked libraries, compiler/runtime and per-IOD data identities. Run each
   pinned tool with the native file path, capture both streams and exit status.
2. Parse with pydicom 3.0.2 `dcmread(path, force=False)`. Do not enable permissive
   recovery or use it to write/repair the fixture. Compare File Meta and dataset
   SOP Class/Instance UIDs and EVR-LE transfer syntax. Compare all selected tags
   against the independent inventory, including empty sequence and Type 2 fields.
3. Compare raw native PixelData bytes to the owned 768-byte RGB fixture or the
   9-byte odd fixture followed by one zero byte. Check shape, samples, interleave,
   unsigned 8-bit allocation/storage/high-bit and never-lossy marker. This native
   layout requires no codec or numpy conversion; no silent image transformation.
4. Independently inspect DICM, meta group length, exact object length and selected
   bytes/offsets from `golden-layout.json`. Require equality across C and C++
   outputs and repeat writes with the same inputs.
5. Run `dciodvfy fixture.dcm` for the standard VL Photographic SOP Class. Zero
   exit is necessary but not sufficient: retain and disposition every warning
   and error. Compare its evolving IOD templates against the reviewed 2026d
   conditions. pydicom's 2024c dictionary and DCMTK's 2025e dictionary do not
   establish current IOD conformance.
6. Negative cases: required element omitted/empty, disallowed non-empty sequence,
   duplicate tag, invalid VR/VM/UID/date, RGB layout mismatch, missing pad, wrong
   SOP/meta identity, truncation, malformed lengths and unsupported syntax.
   Future writer tests must reject bad input before sink writes where feasible.

An unavailable reader or validator is a named evidence gap. Do not replace an
IOD validator with a permissive parser or unexplained warning suppression.
