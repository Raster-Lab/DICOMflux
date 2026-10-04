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

## Licence

MIT. See LICENSE for the full licence text.
