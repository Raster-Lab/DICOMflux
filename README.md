# DICOMflux — Native DICOM Engine

DICOMflux is a from-scratch, MIT-licensed open-source native DICOM engine.

## Design

The engine is designed around a portable C++20 core and an experimental,
versioned C API with C11-readable headers. Android/NDK and hosted Linux,
including Raspberry Pi, are the first target environments.

Platform bindings remain separate from the portable core. DICOMflux is
not a wrapper around another DICOM toolkit.

## Development status

[DF-A002](docs/evidence/authorisations/DF-A002.json) records the owner's narrowed
DF-G0 acceptance and current implementation authority: DF-W05 plus only
`dicomflux_query_abi`, `dicomflux_context_create`, `dicomflux_context_retain`,
`dicomflux_context_release` and `dicomflux_error_copy` from DF-W06.
See the [effective authority index](docs/evidence/effective-authority.json).
Full DF-1, the writer and consumer integration are not authorised in this slice.

The authorised foundation core and five-function C subset are implemented and
host-tested. Use the installed `dicomflux/foundation.h` header and
`DICOMfluxFoundation` CMake package (`DICOMflux::foundation`). See the
[implemented contract](docs/architecture/foundation-core.md) and
[implementation evidence](docs/evidence/foundation-core/README.md).
The four-function DF-0 probe remains a separate prototype; it does not read or
write DICOM. [Historical closure evidence](docs/evidence/g0-closure/README.md)
retains its original unfilled submission fields; subsequent authority is recorded
separately. Linux/Pi/Android and executable-oracle qualification remains open.
There is no engine release, stable ABI or qualified device-support claim.

## Licence

MIT. See LICENSE for the full licence text.
