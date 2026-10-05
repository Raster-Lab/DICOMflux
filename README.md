# DICOMflux — Native DICOM Engine

DICOMflux is a from-scratch, MIT-licensed open-source native DICOM engine.

## Design

The engine is designed around a portable C++20 core and an experimental,
versioned C API with C11-readable headers. Android/NDK and hosted Linux,
including Raspberry Pi, are the first target environments.

Platform bindings remain separate from the portable core. DICOMflux is
not a wrapper around another DICOM toolkit.

## Development status

Foundation stage: DF-0 is authorised. There is no engine release or
qualified device-support claim yet. Planned capabilities must not be
interpreted as implemented features.

The provisional DF-0 review candidate now includes bounded arithmetic/allocation probes,
C11/C++20 install consumers, a Python foundation probe, and qualification
harnesses. It does not read or write DICOM files. See
[DF-0 evidence](docs/evidence/DF-0-report.md),
[reproduction instructions](docs/evidence/reproduce.md), and the
[DF-G0 review request](docs/evidence/DF-G0-review-request.md).
Android, Linux and Raspberry Pi qualification remains incomplete.

**Draft checkpoint: corrections outstanding; DF-G0 is pending.** The review
identified contradictory Type 2 test expectations, incomplete proposed C API
state/error definitions, and Python checks skipped under optimization. See the
[publication status](docs/evidence/publication-status.md) before using results.

## Licence

MIT. See LICENSE for the full licence text.
