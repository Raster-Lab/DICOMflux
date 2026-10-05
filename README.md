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

**Post-merge qualification: DF-G0 remains pending.** PR #1 was merged as a
provisional checkpoint. Fresh host evidence and the proposed narrower gate
disposition are in the [closure submission](docs/evidence/g0-closure/README.md).
Type 2
expectations, the proposed C API state/error contract, optimized Python checks
and macro-count wording have been corrected. See the
[correction results](docs/evidence/corrections/README.md) and
[publication status](docs/evidence/publication-status.md) for remaining gaps.

## Licence

MIT. See LICENSE for the full licence text.
