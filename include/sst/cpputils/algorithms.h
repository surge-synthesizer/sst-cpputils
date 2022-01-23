/*
 * algorithms: A set of utilities for working with containers
 */

#ifndef SST_CPPUTILS_ALGORITHMS_H
#define SST_CPPUTILS_ALGORITHMS_H

#include <algorithm>

namespace sst
{
namespace cpputils
{

/**
 * Wrapper of std::find to check if a container contains a value
 */
template <class InputIt, class T>
constexpr bool contains(InputIt begin, InputIt end, const T &value)
{
    return std::find(begin, end, value) != end;
}

/**
 * Wrapper of std::find_if to check if a container contains a value for which the predicate returns
 * true
 */
template <class InputIt, class UnaryPredicate>
constexpr bool contains_if(InputIt begin, InputIt end, UnaryPredicate q)
{
    return std::find_if(begin, end, q) != end;
}

} // namespace cpputils
} // namespace sst

#endif // SST_CPPUTILS_ALGORITHMS_H
