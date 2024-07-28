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

#ifndef INCLUDE_SST_CPPUTILS_ALGORITHMS_H
#define INCLUDE_SST_CPPUTILS_ALGORITHMS_H

#include <algorithm>

namespace sst
{
namespace cpputils
{

/**
 * Wrapper of std::find to check if a container contains a value.
 */
template <class ContainerType, class T>
constexpr bool contains(const ContainerType &container, const T &value)
{
    return std::find(container.begin(), container.end(), value) != container.end();
}

/**
 * Wrapper of std::find_if to check if a container contains a value for which the predicate returns
 * true.
 */
template <class ContainerType, class UnaryPredicate>
constexpr bool contains_if(const ContainerType &container, UnaryPredicate q)
{
    return std::find_if(container.begin(), container.end(), q) != container.end();
}

/**
 * Similar to std::erase_if, but much easier to use for node-based containers (e.g. std::map).
 */
template <class ContainerType, class UnaryPredicate>
constexpr void nodal_erase_if(ContainerType &container, UnaryPredicate q)
{
    for (auto it = container.begin(); it != container.end();)
    {
        if (q(*it))
            it = container.erase(it);
        else
            ++it;
    }
}

} // namespace cpputils
} // namespace sst

#endif // SST_CPPUTILS_ALGORITHMS_H
