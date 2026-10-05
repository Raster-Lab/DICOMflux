# DICOMflux — repository guidance for coding agents

Current authority is [DF-A002](docs/evidence/authorisations/DF-A002.json), indexed
by [effective-authority.json](docs/evidence/effective-authority.json). DF-G0 is
accepted for the evidenced auxiliary-host foundation only. This task implements
DF-W05 and exactly five DF-W06 functions: dicomflux_query_abi,
dicomflux_context_create, dicomflux_context_retain, dicomflux_context_release,
and dicomflux_error_copy. Full DF-1 is not authorised. Internal cancellation
primitives add no public cancellation API. Preserve the broader candidate as
design material; install declarations only for implemented functions.

Use original MIT-licensed C++20 code and a C11-readable experimental boundary.
Keep the distinct four-function DF-0 prototype and historical evidence intact.
No third-party DICOM toolkit backend, writer, builder/dataset/plan/execution,
codec, networking, scheduler or product integration belongs in this slice.
No new oracle acquisition/build is needed. Consumer and executable-oracle
qualification stays deferred/open; acknowledge historical linkage limits without
retrospective claims. Read the current authorised task and applicable private
baseline/register; this public summary does not grant broader work.

Capture actual checkout, source/test/configuration hashes and exact tools before
new-code tests. Preserve existing checkouts, local work and history. Keep private
control/review inputs, raw paths, patient/product data, credentials, restricted
source and third-party archives outside Git. Use owned synthetic test inputs.
Do not replace historical null/false submission fields with backdated approval.

Only codex/df1-foundation-core may receive audited public-safe commits/pushes
for this task, with one draft PR to main. Do not merge, directly write main,
force-push, reset/rebase history, tag/release/publish packages or change settings.
PR #2 is already merged. Stop after the implementation/evidence PR; its merge
and any further work package require separate authority.
