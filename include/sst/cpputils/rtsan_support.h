/*
 * sst-cpputils - an open source library of things we needed in C++
 * built by Surge Synth Team.
 *
 * Provides a collection of tools useful for writing C++-17 code
 *
 * Copyright 2022-2026, various authors, as described in the GitHub
 * transaction log.
 *
 * sst-cpputils is released under the MIT License found in the "LICENSE"
 * file in the root of this repository
 *
 * All source in sst-cpputils available at
 * https://github.com/surge-synthesizer/sst-cpputils
 */

/*
 * Helpers for clang's RealtimeSanitizer (-fsanitize=realtime).
 *
 * - SST_CPPUTILS_HAS_RTSAN     1 when compiling a realtime-sanitizer build, else undefined/0.
 * - SST_CPPUTILS_NONBLOCKING  tags a function as a realtime context ([[clang::nonblocking]]);
 *                             expands to nothing on any other compiler.
 * - SST_CPPUTILS_RTSAN_DISABLE / SST_CPPUTILS_RTSAN_ENABLE
 *                             declare a scoped RAII guard that suspends / re-asserts rtsan
 *                             checking for the enclosing scope. ENABLE is only meaningful nested
 *                             inside a disabled (or nonblocking) region. Both are no-ops without
 *                             rtsan.
 *
 * Everything here is a no-op unless a realtime-sanitizer build is detected, so the same source
 * compiles and runs identically under any compiler.
 */

#ifndef INCLUDE_SST_CPPUTILS_RTSAN_SUPPORT_H
#define INCLUDE_SST_CPPUTILS_RTSAN_SUPPORT_H

#if defined(__has_feature)
#if __has_feature(realtime_sanitizer)
#define SST_CPPUTILS_HAS_RTSAN 1
#endif
#endif

#if SST_CPPUTILS_HAS_RTSAN

// These live in the rtsan runtime. Newer llvm declares them in <sanitizer/rtsan_interface.h>,
// but llvm 20 does not, so declare them ourselves (a matching redeclaration is harmless).
extern "C" void __rtsan_realtime_enter(void);
extern "C" void __rtsan_realtime_exit(void);
extern "C" void __rtsan_disable(void);
extern "C" void __rtsan_enable(void);

namespace sst::cpputils::rtsan
{
struct ScopedDisabler // suspend rtsan checks for this scope, re-enable on exit
{
    ScopedDisabler() { __rtsan_disable(); }
    ~ScopedDisabler() { __rtsan_enable(); }
    ScopedDisabler(const ScopedDisabler &) = delete;
    ScopedDisabler &operator=(const ScopedDisabler &) = delete;
};
struct ScopedEnabler // re-assert rtsan checks for this scope (use nested in a disabled region)
{
    ScopedEnabler() { __rtsan_enable(); }
    ~ScopedEnabler() { __rtsan_disable(); }
    ScopedEnabler(const ScopedEnabler &) = delete;
    ScopedEnabler &operator=(const ScopedEnabler &) = delete;
};
} // namespace sst::cpputils::rtsan

#define SST_CPPUTILS_NONBLOCKING [[clang::nonblocking]]
#define SST_CPPUTILS_RTSAN_DISABLE ::sst::cpputils::rtsan::ScopedDisabler sstCpputilsRtsanDisabler_
#define SST_CPPUTILS_RTSAN_ENABLE ::sst::cpputils::rtsan::ScopedEnabler sstCpputilsRtsanEnabler_

#else

#define SST_CPPUTILS_NONBLOCKING
#define SST_CPPUTILS_RTSAN_DISABLE (void)0
#define SST_CPPUTILS_RTSAN_ENABLE (void)0

#endif

#endif // INCLUDE_SST_CPPUTILS_RTSAN_SUPPORT_H
