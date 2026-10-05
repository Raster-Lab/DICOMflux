# Binding and target qualification plan

The native arm64 CPython 3.12.14 ctypes foundation probe ran against the relocated
installed library with explicit function signatures, ABI size/alignment checks,
load/call, forced GC, invalid options, overflow, context-manager cleanup and
idempotent wrapper close. It does not exercise callbacks or write a file.

JNI load/close was not built or run: this host has no usable JDK, Android SDK,
NDK, AGP, Kotlin, emulator or device. No uncompiled Java shim is counted as
evidence. Select exact versions together with the target before closing this
gate. NDK r30 30.0.16248370 and minSdk 23 remain candidates, not qualified floors.
The user supplied Raspberry Pi targets but no Android device/minimum API.

The next JNI foundation witness must use one shared object, incorporated owned
probe and a private static C++ runtime, with only reviewed JNI registration
symbols exposed. Inspect ELF exports/dependencies and 16 KB load-segment
alignment, then test actual APK alignment and separate 4 KB/16 KB environments.
A macOS Mach-O dependency check says nothing about Android packaging.

JNI wrappers need a checked token registry, explicit idempotent close and a
retained per-operation lease. Reject stale tokens, do not cast arbitrary jlong
to pointer, do not share JNIEnv across threads, and bound direct-buffer accesses.
Convert callback exceptions to native statuses without releasing the owner
before synchronous terminal completion. A later real-writer binding must test
CheckJNI, GC, close/cancel races and callback failure on physical ARM separately
from the emulator. None of these future acceptance tests has run.

The requested Pi matrix is Pi 4, Pi 5, CM4 and CM5 × Bullseye, Bookworm and
Trixie, using the baseline AArch64 candidate. Each needs an exact OS image hash,
board revision, kernel/page size, compiler/stdlib/glibc/sysroot and Python pin.
No image or board was available. No 32-bit support is inferred. CM5/Bullseye
conflicts with [official CM5 guidance](https://www.raspberrypi.com/documentation/computers/compute-module.html)
requiring Bookworm or later, kernel 6.12.x or later and firmware from 2025-03-10.
Pi 5/Bullseye also needs explicit supported-image compatibility resolution.

Relevant official qualification inputs: [NDK downloads](https://developer.android.com/ndk/downloads),
[middleware runtime guidance](https://developer.android.com/ndk/guides/middleware-vendors),
[16 KB pages](https://developer.android.com/guide/practices/page-sizes), and
[ctypes](https://docs.python.org/3.12/library/ctypes.html).
