# DF-0 bounded correction results

DF0-REV-001, 002, 003 and 007 are corrected and submitted for review. DF-G0 is
pending human disposition. Findings 004–006 retain qualification, provenance and
historical evidence limits. No writer/parser or DF-1 engine operation was added.
See [finding dispositions](finding-dispositions.json) and [gate request](../df-g0-review-request.json).

The provisional remote checkpoint is 59d465f49f3caaa9a7e89b28f92222b4752d068a
(local equivalent 82c339641e0d486e14f8064c84fc698a5df6a55e; identical tree).
This correction follows it without replacing the original checkpoint or logs.
GitHub publication uses the authenticated app because local HTTPS credentials
are unavailable. Local/remote commit identities differ; tree equality is verified.

## Fresh checks

The `df0-corrections-v1` records contain 24 successful commands: exact tool
versions, fresh build, three test executables (89 native foundation checks and
two proposed-contract consumers), installation/relocation, two installed clients,
normal/optimized Python and binary checks, and document/evidence checks. Seven
regression tests passed under normal Python and again under -O. Their subprocess
cases prove exit 1 for invalid native budgets and contradictory profile/evidence
inputs in both modes; expected rejections are test successes, not hidden failures.
The normal, -O and PYTHONOPTIMIZE=1 probe observations all report ABI 80/8.

After adding fresh run identifiers and collision checks, the actual public
`run_probe_suite.py` host path also passed (`suite-host-v1`). Its exact record set
includes fresh source snapshots, build/install/consumer/Python/regression/binary/
fixture/resource checks. It preserves old output directories and requires an empty
evidence directory. ASan/UBSan/TSan and fuzz were not rerun for these contract and
harness changes; historical campaigns remain separately labelled.

Observed tools: Apple Clang 21.0.0 (clang-2100.0.123.102), CMake 4.4.4,
Ninja 1.13.2.git.kitware.jobserver-pipe-1, CPython 3.12.14, SDK 26.4,
macOS 27.0.1 arm64. Compiler/runtime identities are observations, not minimum
version qualification. No new dependency was downloaded or executed.

Each run directory contains pre-execution input hashes and exact sanitized
commands, timestamps, outputs and exit statuses. The first full input map includes
historical reports at run time; later report/provenance edits do not change tested
code. `current-tested-inputs.json` binds the public code/config/specification inputs
to the later suite capture, followed by a targeted regression-only change that
retains each negative input under a unique filename. That final regression script
passed in normal and -O modes with inputs captured in `final-targeted/` before
execution. Native source and owned raw pixels remain unchanged.
Unredacted commands, source byte snapshots and negative-case inputs/logs remain
private. Sanitized results and factual hash manifests are public. Historical
source hashes are checked at the checkpoint, not overwritten with new hashes.

## Remaining gates

All 12 requested Pi board/OS targets remain unrun. The
[Pi 5 FAQ](https://www.raspberrypi.com/products/raspberry-pi-5/) excludes OS versions
older than Bookworm. [CM5 guidance](https://www.raspberrypi.com/documentation/computers/compute-module.html)
also requires Bookworm or later with the stated kernel/firmware prerequisites.
Those two Bullseye requests need human disposition. No requested target was
silently substituted; Linux/Android/JNI/page-size witnesses are still absent.

DCMTK remains quarantined: the original bytes still hash to f103df876040a4f904f01d2464f7868b4feb659d8cd3f46a5f1f61aa440be415.
The live [OFFIS files page](https://support.dcmtk.org/redmine/projects/dcmtk/files)
redirected to a challenge; its search index repeats the differing historical hash.
The [distribution index](https://dicom.offis.de/download/dcmtk/current/) names the
archive but supplies no digest. This does not reconcile the difference.
No oracle validation or 2026d conformance is claimed. Standard captures/extracted
text and toolkit archives stay private; project MIT does not relicense them.

Historical input/corpus gaps cannot be recreated. All 60 historical command
records, including 7 failures, remain intact. Only 28 carry the final build digest;
32 do not. New input capture is new evidence, not a repair of missing history.
Future corpus snapshot code is present but no fresh fuzz witness is asserted.

## Reproduction

Use public build/test/specification files and the tool identities above. No private
approval/control document is needed to compile the probe. Obtain external tools
from their reviewed sources; the suite performs no downloads. For a bounded host
rerun, pass explicit tool paths, `--configs host`, a fresh `--run-id`, and an empty
private `--evidence-dir` to `tools/run_probe_suite.py`. Run
`python3 -O tests/regression/test_df0_corrections.py --library <relocated-library>`
for the optimized regression entry. Validate current identities with
`python3 tools/validate_evidence.py --input-manifest docs/evidence/corrections/current-tested-inputs.json`.
Keep checkpoint Git history available (not a shallow checkout missing that commit).
Exact original source snapshots, private paths, historical corpus and captures
are not public inputs; complete historical replay is not claimed.
