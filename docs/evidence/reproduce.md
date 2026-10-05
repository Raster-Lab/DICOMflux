# Reproducing the auxiliary host evidence

Use a private evidence directory outside Git. Install/acquire only the exact
reviewed tools in the lock/manifest; the runner fetches nothing. This run used
Apple Clang at /usr/bin/clang and /usr/bin/clang++, so the runner is currently
a macOS host runner, not a qualified Linux/Android automation. CMake targets
remain separate from the platform-specific runner.

```sh
python3 tools/run_probe_suite.py --cmake /absolute/path/to/cmake \
  --ninja /absolute/path/to/ninja --python /absolute/path/to/python3.12 \
  --evidence-dir /absolute/private/evidence \
  --fuzzer-runtime /absolute/path/to/pinned/libFuzzer.a
```

Build/install/relocation outputs are under ignored build/, install/ and stage/.
Fresh consumer sources are copied, configured separately against installed
headers and CMake targets, then compiled as C11 and C++20. No private internal
header is available to those clients. Native C++ runtime linkage is recorded.

The supplied native toolchain lacked libFuzzer's runtime archive. A standalone
compiler-rt 21.1.8 archive was compiled with Apple Clang, C++17 and -O2; exact
commands and warnings are in private evidence. Conditional empty-object archive
warnings were inspected. Test-runtime code is never linked to the shipped
prototype library. The two recorded campaigns use deterministic seeds
20261004/20261005, max_len 64, requested 10/30 seconds and a persisted corpus.
Corpus hashes identify this continuation run; it did not start from six seeds
alone. The final on-disk corpus is retained; the exact pre-campaign corpus was
not separately snapshotted, so bit-for-bit replay of this timed continuation
is not claimed. Sanitizers are independent builds. The runner propagates nonzero exits.

Public records substitute ${SOURCE}, ${PRIVATE_WORK}, ${CONTROL}, ${SESSION}
and ${PYTHON_RUNTIME} for local roots. Private records retain exact commands,
cwd, timestamps, stdout/stderr and the substitution map. A build-source digest
identifies tested code; uncommitted HEAD alone does not. Commands are evidence,
not approvals to run later slices.

One resource wrapper failed because /usr/bin/time -l could not access a kernel
sysctl. Its child passed, but the wrapper failure and final suite exit 1 remain
recorded. A separate corrected measurement used getrusage for one child and
succeeded; no failed result was overwritten. The runner now uses this method.
Profile/File Meta document checks were repeated after specification completion;
the native build inputs remained byte-identical to the final tested hashes.

## Correction follow-up

The current runner gives each run fresh build/install/stage paths (`--run-id`)
and rejects existing outputs or nonempty evidence directories. Source/test/config
bytes and identities are frozen before execution. Corpus snapshots are saved
before each future fuzz campaign and afterward; this does not recreate historical
initial corpora. Use `--configs host` for the bounded correction checks and see
`corrections/README.md` for normal/-O regression and current-source validation.
