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
template <class ContainerType, class T>
constexpr bool contains(const ContainerType& container, const T &value)
{
    return std::find(container.begin(), container.end(), value) != container.end();
}

/**
 * Wrapper of std::find_if to check if a container contains a value for which the predicate returns
 * true
 */
template <class ContainerType, class UnaryPredicate>
constexpr bool contains_if(const ContainerType& container, UnaryPredicate q)
{
    return std::find_if(container.begin(), container.end(), q) != container.end();
}

} // namespace cpputils
} // namespace sst

#endif // SST_CPPUTILS_ALGORITHMS_H
