# DF-0 post-merge qualification and DF-G0 submission

Recommend **narrower auxiliary-host foundation acceptance**, subject to human
disposition. Complete DF-0 target qualification is not established. Progression
is stopped; `human_disposition` is null and `df1_authorised` is false.

## Adopted source and evidence boundary

The clean closure worktree started from merged main
`cf17269a3af7af503b30c8f6310b492dd58fa302`, tree
`e007c8554228fd8c759891c3218d0ad7be1f2b21`, identical to reviewed PR #1 head
`26be4ee0f904e0b538d474b59e90a292a12d78d5`. Live remote/merge inspection preceded
implementation. The original local checkout, divergent commits and two untracked
private metadata files remain preserved; no reset or rebase was used.
[Adoption record](adoption.json) and [182-file source manifest](adopted-source-files.json)
bind this starting point. Actual source/control paths and the readable approval,
baseline, effective register and work-order hashes stay in the private DF-W00 record.
The read-only handover verifier passed all 32 frozen inputs in its own directory.

[Manifest](manifest.json) links command records, hashes and classifications.
`host-run/build-source-files.json` identifies source/test/configuration bytes
captured before execution. Private snapshots retain those bytes. Follow-up
input manifests were also written before their commands. Public paths use
`${SOURCE}`, `${ORIGINAL_SOURCE}`, `${PRIVATE_WORK}`, `${PRIVATE_CLOSURE}`,
`${PYTHON_RUNTIME}` and `${SESSION}` substitutions; the private map and logs
retain actual paths. Hashes of original path-bearing records are distinct from
hashes of their sanitised public forms.

## Work-order status

| Work | Closure result | Evidence |
|---|---|---|
| DF-W00 | Adopted verified merge; original work/history preserved; controls readable; public/private boundary audited | `adoption.json`, `adopted-source-files.json` |
| DF-W01 | Fresh auxiliary macOS arm64 build/install/relocation/C11/C++20/Python witness passed; consumer targets open | `host-run/`, `extra-run/`, `target-selection.json`, existing target matrix |
| DF-W02 | Profile and Type 2 checks retained; selected pydicom VR/VM coverage and strict rejection witnessed; second reader/IOD setup incomplete | `oracle-run/`, `oracle-provenance.json`, `oracle-plan.md` |
| DF-W03 | Corrected candidate C API compiles for independent C11/C++20 clients; no engine implementation or ABI release claimed | `host-run/host-unit.json`, `host-run/consumer-run.json`, unchanged API proposal/input hashes |
| DF-W04 | Seven correction regressions pass in normal and optimized parent modes; negative exits retained; historical assurance limits acknowledged | `host-run/correction-regressions.json`, `extra-run/regressions-optimized.json`, `regression-children/` |

## Fresh execution results

The existing host suite ran once in fresh `g0-adopted-main` output directories:
**20/20 commands passed**. Native tests report 89 checks, two allocation sites
with two failure modes, and 4,000 concurrent measurements. Three native/C-contract
CTest tests and two independently installed-consumer tests passed. Python normal
and `-O` loads/calls/GC/explicit close/invalid options passed with ABI size/alignment
80/8. Only four reviewed probe symbols are exported. These are foundation probes.

Observed host: macOS 27.0.1 arm64, Apple Clang 21, SDK 26.4, CPython 3.12.14,
exact CMake/Ninja versions and binary hashes in the records. Standard-library
macros and native dependencies are captured separately. `getconf PAGESIZE`
reported 16,384 bytes on this Mac; it is not an Android 16 KB witness.
One resource observation reports library tracked peak 65,648 bytes and process
RSS 2,195,456 bytes; this is not a writer benchmark or target performance claim.

Evidence validators passed normally and with `-O`; profile and Python checks ran
in both modes inside the seven regressions. All 24 recorded regression child
commands match their expected exits, including 16 non-zero deliberate rejection
cases. Additional binary-policy optimized checking passed. The new isolated
oracle checker passed normally and with `-O` and rejects mismatched wheel hashes
before import in both modes. Two initial direct-wheel import failures are retained;
unpacking the hash-verified wheel into a new private directory resolved its need
for filesystem resources. No network was allowed by the final setup checker.
The Java stub returned exit 1 because no runtime exists: that is an unavailable
prerequisite, not successful JNI qualification.

No native source changed, so no new fuzz/sanitizer campaign was justified.
The existing runner still saves source/config bytes and initial/intermediate/final
corpus identities prospectively. Its earlier 60 command records, seven failures,
32 unlinked runs and missing initial corpus links remain unchanged. Fresh runs
do not identify those historical inputs retroactively. All 24 engine acceptance
entries remain `not_run`.

## Findings

| Finding | Result and remaining limit |
|---|---|
| DF0-REV-001 | Retained correction; all 15 Type 2/2C prescriptions and both legal-empty fixture witnesses protected by mutation regressions. |
| DF0-REV-002 | Retained fixed-width state/query/error contract; C11/C++20 clients pass. Candidate API only. |
| DF0-REV-003 | Normal/optimized real calls and explicit checks pass; contradictory records/profile/budgets reject with exit 1. Binary-policy mutations reject extra symbols/dependencies. |
| DF0-REV-004 | Open. All 12 Pi requests preserved: eight current OS candidates, two older-image/access gaps, two incompatible Bullseye requests. No Linux/Pi/Android/JNI execution here. |
| DF0-REV-005 | Partial advancement. All 3,516 DCMTK archive regular files match official release Git blobs; archive digest conflict remains and bytes stay quarantined. Pydicom 38/38 selected VR/VM matches; dicom3tools template presence checked, no executable IOD run. |
| DF0-REV-006 | Acknowledgement proposed for unrecoverable historical linkage limits; prospective capture retained. Administrative closure is not historical verification. |
| DF0-REV-007 | Retained correction: 19 tables, 10 macro invocations, nine distinct macro tables; profile checks pass. |

The [PR #1 technical review](https://github.com/Raster-Lab/DICOMflux/pull/1#pullrequestreview-5411436831)
reports a five-file direct-GCC Linux x86_64 probe. It is external reported evidence,
not rerun here; its raw artifacts were not supplied. It does not establish a
complete repository build, CMake/install consumers, Linux platform qualification
or DF0-REV-004 closure. No whole-suite pass is inferred.

## Decisions requested

| Human decision | Recommendation | Consequence |
|---|---|---|
| Initial consumer targets | First select Pi 4 / official Bookworm Legacy Lite 64-bit image pinned in `target-selection.json`; retain other requests. Disposition Pi 5/CM5 Bullseye as incompatible. Confirm Android device/build and existing minSdk 23 candidate. | Actual Linux/Pi and JNI/page-size evidence remains required before corresponding consumer support claims. Twelve requests are not twelve invented mandatory gates. |
| Oracle admission | Continue isolated pydicom; keep disputed bytes quarantined. Prefer a fresh official commit-pinned DCMTK source, independently built, plus dicom3tools `dciodvfy`. | No toolkit backend. New source rights/digests and executable diagnostics must be reviewed before future object acceptance. |
| Historical limit | Acknowledge DF0-REV-006 administratively; preserve the original failed/unlinked logs. | Fresh evidence is usable without a claim of retrospective verification. |
| Exact progression scope | If accepting the narrower host foundation, separately authorise DF-W05 plus only ABI query/context/error-copy/release from DF-W06 on the observed host. | Stop before builder/dataset/plan/execution APIs, writer/profile work, JNI/Pi integration and later work orders. No authority is granted by this recommendation or a merge. |

Machine-readable decisions, all unfilled, are in [human-decisions.json](human-decisions.json).
Complete acceptance is unsupported by missing consumer/oracle execution evidence.
A continued hold is appropriate if the human reviewer requires those witnesses
before any host-only foundation work, or identifies a material licence/privacy/ABI/
profile conflict. No new such conflict was established here. The recommended
narrower boundary avoids a support claim and does not require DF-1 writer tests
to pass before DF-0. The nine accepted architecture proposals are not reopened.

## Reproduction

Use the exact binaries identified in `host-run/environment.json` and the version
records, with private paths substituted locally. From the adopted checkout:

```sh
python3 tools/run_probe_suite.py --cmake /absolute/path/to/cmake \
  --ninja /absolute/path/to/ninja --python /absolute/path/to/python3.12 \
  --evidence-dir /absolute/private/fresh-host-evidence \
  --configs host --run-id unique-host-closure
python3 tools/validate_evidence.py --input-manifest /absolute/private/fresh-host-evidence/build-source-files.json
python3 -O tools/validate_evidence.py --input-manifest /absolute/private/fresh-host-evidence/build-source-files.json
```

The optimized regression, binary checks and environment observations have exact
arguments in `extra-run/`; oracle setup instructions are in [oracle-plan.md](oracle-plan.md).
This submission adds only closure documentation/evidence and one isolated oracle
qualification helper. It implements no writer, parser, codec, network service,
later work order or product integration.
