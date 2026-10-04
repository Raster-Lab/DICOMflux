# DICOMflux — repository guidance for coding agents

DICOMflux is a separate, from-scratch, MIT-licensed native DICOM engine.
Implement the engine in C++20 with an experimental versioned C11-readable
boundary. Keep platform bindings and optional dependencies outside the
portable foundation. Do not use another DICOM toolkit as an implementation
backend. External tools may be isolated test oracles only.

Read the current authorised task and its privately supplied baseline,
approval addendum and work order before changing code. This public file
is not a substitute for that task authority and does not grant new work.

Current commissioned slice: DF-0, DF-W00 through DF-W04 only. Produce real
bounded foundation probes and meaningful harnesses where the work order
requires them. Record exact observed versions and results. Candidate
platform floors and toolchains are not qualified by being named.

Inspect the actual checkout and preserve pre-existing changes. Keep the
private control package outside Git. Never import private product code,
patient data, credentials or production endpoints. Use owned synthetic
fixtures and record permitted third-party provenance.

Submit the DF-0 evidence package and DF-G0 disposition request. Stop before
DF-1 unless a later explicit authorisation permits progression. Do not
self-approve gates, claim absent device runs, or equate scaffolding with a
working engine.

Do not initialise or create repositories, alter remotes or visibility,
commit, push, publish packages, or change another product without explicit
permission for the operation. The user's repository setup is not an
extension of the engineering scope. Return local changes and evidence
under the current authorised task.
