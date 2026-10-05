# DF-A002 foundation-core implementation evidence

**DF-W05 is implemented and verified for the authorised auxiliary-host scope.**
Only the five-function DF-W06 subset was implemented. Full DF-1, writer/profile
work, consumer integration and executable-oracle object acceptance remain outside
this task. DF-G0 was already accepted narrowly under
[DF-A002](../authorisations/DF-A002.json); this submission does not request that
same approval again and does not authorise merging its draft PR.

## Source and implementation

Adopted merge: `7f99e34a010f922b8bf85404840bc744a7af66fc`, tree
`0393a9676b9b8985dfd8a0ed56930dc1e2bc5af3`. The live PR #2 merge and public
[owner-disposition record](https://github.com/Raster-Lab/DICOMflux/pull/2#issuecomment-5994925456)
were verified before edits. A separate `codex/df1-foundation-core` worktree
preserves both earlier checkouts and the original divergent local history.
[Adoption](adoption.json) records the public-safe identity; actual paths and
private control hashes remain separately retained.

The real foundation is distinct from the unchanged four-function DF-0 probe.
It implements checked arithmetic/conversions, bounded structured errors,
allocator admission/rollback, atomic reference ownership, internal retained
storage and cancellation. The installed package `DICOMfluxFoundation` exports
target `DICOMflux::foundation`, header `dicomflux/foundation.h`, and shared
library `dicomflux_foundation`. Its only C exports are:

```text
dicomflux_query_abi
dicomflux_context_create
dicomflux_context_retain
dicomflux_context_release
dicomflux_error_copy
```

See the [implemented contract](../../architecture/foundation-core.md). The broader
candidate header/contract stays unchanged design material and is not installed as
an implemented API. There are no public cancellation or capabilities functions.
The generated build-capability JSON lists the actual five functions and absent
image/codec/network capabilities; DF-R004 is satisfied only to this limited extent.

The local governance/implementation commits are distinct from their GitHub-created
counterparts because publication metadata differs. Both histories are preserved;
no reset/rebase/force operation reconciles them. [Implementation identities](implemented-inputs.json)
bind local `8cafba65fa0f7a91ccf3f336486094c0faaf08bb` and remote
`324320da66c2ce116286a61a601667d23917abb7` to identical tree
`ab1b08f42d58177c3dd9ce739ec6cc2ecf5d3e71`. The following assurance commit adds
documentation and evidence without changing those compiled/tested inputs.

## Actual results

Final native tests report **394 checks**, three allocation sites, four injected
failure modes, 8,000 concurrent reference pairs, 400 concurrent owned blocks and
four independent contexts. Context storage is 176 bytes on this host; installed
options/error record sizes are 200/328 bytes. Those sizes are observations, not
cross-platform ABI constants or product limits.

| Case | Result | Main evidence |
|---|---|---|
| FC-01 | Fresh real foundation build, C++20/C11, captured source/tool identity | `run-03/identity-before.json`, `host-build.json` |
| FC-02 | Relocated install; separate C11/C++20 clients call all five functions | `run-03/installed-consumers.json`, `consumer-compile-commands.json` |
| FC-03 | Outer/nested size/version/flag/reserved negatives and guarded outputs pass | `run-03/host-tests.json`, `run-02/address-tests.json` |
| FC-04 | Sentinels, empty/max/truncated text, query/exact/insufficient copies pass | `run-03/host-tests.json` |
| FC-05 | Zero/limit/overflow/narrowing/signed boundaries; arithmetic vs budget errors pass | `run-03/host-tests.json`, `run-02/undefined-tests.json` |
| FC-06 | NULL, bad_alloc, controlled C++ exception and misalignment at all three allocation sites; rollback and pairing pass | `run-03/host-tests.json`, `run-02/address-tests.json` |
| FC-07 | Context references and internal owners retain copied allocator/budget state until final release | `run-03/host-tests.json` |
| FC-08 | Legitimate reference/cancellation/allocation concurrency and independent contexts pass | `run-02/thread-tests.json` |
| FC-09 | Exactly five foundation exports; libc++/libSystem dependencies; four probe exports separately preserved | `run-03/boundary-normal.json`, `probe-boundary.json` |
| FC-10 | Existing DF-0 native/C-contract tests and seven regressions per normal/optimized mode remain protected; new Python checks reject invalid inputs | `run-03/df0-regressions-*.json`, `boundary-*.json`, `helper-negatives/` |
| FC-11 | ASan, UBSan, TSan and meaningful bounded coverage-guided fuzzing pass | `run-02/*-tests.json`, `fuzz-*.json`, `owned-corpus-bytes.json` |
| FC-12 | Effective authority and five-function capability scope checked; prior submissions remain historical | `run-03/boundary-*.json`, `../effective-authority.json` |

The [machine-readable matrix](acceptance-matrix.json) gives exact paths and
requirement mappings. “Passed” is confined to the demonstrated foundation case;
it does not promote the complete requirements register or the 24 historical
engine acceptance entries, which remain `not_run`.

Observed tools: macOS 27.0.1 arm64, Apple Clang 21, libc++ `_LIBCPP_VERSION`
210106, SDK 26.4, CMake 4.4.4, Ninja 1.13.2.git.kitware.jobserver-pipe-1 and
CPython 3.12.14. Exact compiler strings, executable hashes, flags, interpreter,
SDK, OS and 16,384-byte host page size are in the records. The previously built,
pinned compiler-rt libFuzzer archive was reused without upgrading tools. Its hash
is in the pre-execution identity record. No sanitizer/runtime was unavailable here.

The final instrumented native run is `run-02`; the final host/Python inspection
run is `run-03`. All native, build, consumer and fuzz input bytes match both.
Run-02 exercised the runner with Python `-O`; run-03 exercised normal mode after
adding missing-source-identity rejection to the inspection helper. Both modes
of the final inspection helper reject unauthorized capability claims and an empty
source hash set. The final runner also rejects an empty configuration in normal
and `-O` before creating any output. New Python acceptance checks use explicit
exceptions, not removable assertions.

Run-02 contains 36 successful commands and two expected non-zero rejections;
run-03 contains 23 successful commands and four expected rejections. Retained
regression children contribute 32 further expected rejection exits across these
two runs; the runner-negative checks contribute two. The manifest has **75 passed
command records and 40 expected rejections, with no failed positive command**.
Parent suites and child invocations overlap; these counts are not independent
coverage measures. All stdout/stderr and expected/actual exits are retained.
The initial successful 384-check run remains separately preserved and summarized
in `initial-run-summary.json`; its inputs were not retroactively relabelled.

The final new-code fuzz campaigns used seeds 20261005/20261006, `max_len=96`, and
requested 5/15-second limits. Actual libFuzzer reports were **2,881,019 executions
in six seconds** and **7,936,386 in sixteen seconds**, with no crash/sanitizer finding.
Initial, continuation and final corpus hashes and owned bytes are in `run-02/`.
Finite runs do not prove memory safety or deterministic replay of timing/iterations.
The instrumented fuzzer's reported peak process RSS (485/490 MB) includes sanitizer
and harness runtime; it is not the library's tracked allocation counter or a
writer performance result. Controlled allocation callbacks cap each fuzz allocation
at 4,096 bytes and reject inconsistent outcomes; arbitrary pointers are not fuzzed.

## Limits and preserved boundaries

- Experimental ABI only. Valid naturally aligned, non-overlapping caller memory,
  live references and valid callback/user-state lifetimes are still required.
- The void retain API cannot report reference-count exhaustion. It terminates
  rather than wrap or fabricate ownership. A throwing deallocator likewise violates
  the host contract; arbitrary foreign unwinding is not supported.
- Only tracked storage is consumed here. Future metadata/item/fragment/codec/
  writer counters are copied settings, not advertised as implemented workloads.
- No DICOM object, dataset, builder, plan, writer, reader/parser, codec, scheduler,
  network service or product integration was added. External oracles were neither
  acquired nor built nor executed for object validation. DCMTK quarantine remains.
- Linux/Pi/Android/JNI, deployment floors and physical/emulator page-size gates stay
  deferred/open. This auxiliary Mac witness does not substitute for them.
- Historical DF-0 logs, failures, missing source/corpus links, submitted gate
  proposals and the broader candidate contract remain unchanged. DF0-REV-006's
  acknowledgement does not verify those historical inputs retroactively.

## Reproduction and private evidence

The runner fetches nothing. Supply the exact tool binaries listed in the identity
record, a fresh ignored output prefix, and a new evidence directory outside Git:

```sh
python3 tools/run_foundation_suite.py --cmake /tools/cmake --ninja /tools/ninja \
  --python /tools/python3.12 --fuzzer-runtime /tools/libFuzzer.a \
  --evidence-dir /private/fresh-foundation-evidence --run-id unique-foundation-run
```

`--configs host` selects only the host/installed-consumer checks. The default adds
separate address, undefined, thread and fuzz builds. The original install prefix
is renamed and retained after copying to the relocated stage; independent clients
use only that stage's exported package/header. Source include leakage is checked.

Source/config/test bytes are copied and hashed before execution, together with
Git HEAD/tree and dirty/untracked state. Final artifact hashes are in
[binary-manifest.json](binary-manifest.json); binaries stay local.
[Manifest](manifest.json) verifies sanitised public evidence bytes. Public records
replace actual roots with `${SOURCE}`, `${PRIVATE_TASK}`, `${PRIVATE_WORK}`,
`${SESSION}` and `${PYTHON_RUNTIME}`. The private supporting index retains actual
checkout/control paths, raw logs, snapshots, audit and publication mapping. No
private control/approval bundle, archive, standards prose or raw local path is
part of the public commit set.
