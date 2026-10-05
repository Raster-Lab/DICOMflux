# Isolated reader and IOD qualification plan

The [existing oracle manifest](../../../third_party/manifest/oracles.json) remains
the version/rights record; this is a supplement, not a replacement baseline.
No oracle is a production dependency. Native DICOM objects do not yet exist.

## Witnessed pydicom setup

Pydicom 3.0.2's wheel SHA-256 was reconfirmed against the
[PyPI release metadata](https://pypi.org/pypi/pydicom/3.0.2/json). MIT and the
included GDCM-derived private-dictionary terms remain with the private wheel.
Nothing from its dictionary or bundled fixtures is copied into Git.

Acquire the manifest-pinned wheel separately. With the exact recorded Python:

```sh
python3 -I -S tools/check_reader_dictionary.py --wheel /private/oracles/pydicom-3.0.2-py3-none-any.whl --scratch /private/new-pydicom-normal
python3 -I -S -O tools/check_reader_dictionary.py --wheel /private/oracles/pydicom-3.0.2-py3-none-any.whl --scratch /private/new-pydicom-optimized
```

Use a different nonexistent scratch directory each time, outside the checkout.
The helper checks the wheel hash before importing it, unpacks it privately,
uses isolated Python without site packages, verifies module origin/version, and
rejects network connection/name-resolution attempts. Invalid wheel hashes exit 1.
The first direct-wheel import approach failed on filesystem resources; both
failures remain in `extra-run/`. Final unpacked results are in `oracle-run/`.

The 31 unique selected dataset tags plus seven file-meta tags agree on VR/VM,
including the OB/OW alternatives. Pydicom's dictionary edition label is 2024c;
this concrete check, rather than the label, supports selected-tag lookup. Strict
reading rejects the owned invalid non-Part10 bytes; see
[official reader documentation](https://pydicom.github.io/pydicom/stable/tutorials/dataset_basics.html).
This neither tests a positive object nor validates IOD Type/conditions, nested
content, omitted attributes, encoding, pixels or the entire 2026d standard.

## DCMTK: keep quarantine; propose a clean pinned source

[Official release](https://github.com/DCMTK/dcmtk/releases/tag/DCMTK-3.7.0)
tag object `dd841c3a858dfb20cfa8574a2a54a33d032b0de4` resolves to commit
`ccfd10b84ff3c9a40b7b331698aedf06d421fc43`, tree
`685ac871c51d663e6a6843db13877f11a3d0211b`. The tag is unsigned. HTTPS API
retrieval supplies upstream provenance, not a signature or archive attestation.
For every regular archive entry, a read-only comparison calculated
`SHA1("blob " + decimal_length + NUL + bytes)` and compared it with the pinned
official Git tree. All 3,516 agree; `.gitignore` is present only in Git.

The two historical archive digests still differ. Release metadata lists no source
archive checksum, and OFFIS web access returned a challenge. Neither blob equality
nor another local hash changes admission of the disputed bytes. No DCMTK code was
executed. Proposed alternative: acquire a **new** snapshot of that official commit,
record its archive and file identities, inspect retained component licence terms,
then build `dcmdump` in a private standalone test prefix. Record exact CMake,
compiler, options, optional dependency choices, executable version/hash and
diagnostics. A system/global install or production linkage is unnecessary.

## dicom3tools IOD checker

The pinned `1.00.snapshot.20260901072548` archive was inspected as data only.
Its hash is locally observed, not an upstream signature. The
[official project](https://www.dclunie.com/dicom3tools.html) describes BSD-style
licensing and `dciodvfy`; retain its COPYRIGHT and component terms privately.

Read-only inspection of `libsrc/standard/elmdict/dicom3.tpl` found all 38 selected
tags: 37 literal VR/VM matches and equivalent `OW/OB` alternatives for Pixel Data.
`libsrc/standard/iodcomp/vl.tpl` includes `VLPhotographicImage` and the applicable
module identifiers. Exact inspected-file hashes and identifiers are in
`oracle-run/dicom3tools-static-coverage.json`. This is source presence evidence;
the checker was not built, its conditional/macro logic was not proven complete,
and no warnings/errors were adjudicated. Template presence cannot validate an IOD.

The local environment lacks `imake`, `makedepend` and `mkdirhier`. Before an
isolated build, pin those prerequisites and the compiler, record their rights and
hashes, then follow that archive's INSTALL using a private `InstallInTopDir`
prefix. Keep actual commands and any configure/build failures. Do not install to
system directories or generate objects with another toolkit. Capture `dciodvfy`
and `dcdump` executable identities; no successful build is claimed here.

If clean DCMTK admission is declined, `dicom3tools dcdump` is a proposed second
reader alongside pydicom. It is independently sourced from pydicom but shares
source/templates with `dciodvfy`, reducing oracle diversity. This alternative is
unapproved and unbuilt, not a silent substitution.

## Future object checks, explicitly unrun

When separately authorised native writer output exists, retain the owned fixed
16×16 RGB and odd-payload objects with hashes. Feed identical bytes to two admitted
strict readers; compare selected tags, SOP identities, exact decoded native samples
and independently specified offsets/padding. Use no force-read fallback and no
toolkit-generated output as native-engine evidence. Run pinned `dciodvfy`, retain
every diagnostic/exit and classify each against the permitted reviewed profile.
Exercise missing Type 1/2 elements, legal-empty Type 2 sequence/orientation, wrong
VR/VM and malformed lengths as independent negative objects. Record conditional
coverage explicitly; diagnose edition differences attribute by attribute.

These are DF-1 object-acceptance plans, not DF-0 execution results or prerequisites
that have already passed. Standards captures and extracted prose remain private;
future generated-data work must identify permitted immutable inputs first.
