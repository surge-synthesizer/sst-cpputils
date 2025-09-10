/*
 * sst-cpputils - an open source library of things we needed in C++
 * built by Surge Synth Team.
 *
 * Provides a collection of tools useful for writing C++-17 code
 *
 * Copyright 2022-2024, various authors, as described in the GitHub
 * transaction log.
 *
 * sst-cpputils is released under the MIT License found in the "LICENSE"
 * file in the root of this repository
 *
 * All source in sst-cpputils available at
 * https://github.com/surge-synthesizer/sst-cpputils
 */

#include "sst/cpputils/algorithms.h"
#include "sst/cpputils/iterators.h"
#include "sst/cpputils/lru_cache.h"
#include "sst/cpputils/ring_buffer.h"
#include "sst/cpputils/bindings.h"
#include "sst/cpputils/constructors.h"
#include "sst/cpputils/fixed_allocater.h"
#include "sst/cpputils/active_set_overlay.h"

static_assert(__cplusplus >= 202002L, "Surge team libraries have moved to C++ 20");