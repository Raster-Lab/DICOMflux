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

For the explicitly authorised DF-0 reconciliation task, reviewed public-safe
files may be committed and pushed to codex/df0-foundation-review, with a draft
pull request targeting main. This narrow exception permits no private materials,
direct-main writes, force-pushes, history rewriting, merges, releases, tags,
repository setting changes or DF-1 work. Other repository operations require
explicit task authority. DF-G0 remains pending human disposition.
