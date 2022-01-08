/*
 * iterators: A set of utilities to make iteration a bit more useful
 */

#ifndef SST_CPPUTILS_ITERATORS_H
#define SST_CPPUTILS_ITERATORS_H

#include <tuple>

namespace sst
{
namespace cpputils
{

/*
 * enumerate allows structured bindings of iterators. A typical usage would be
 *
 * ```
 * std::vector<int> v{7,14,21};
 * for (const auto [idx, val] : v)
 * {
 *     assert((idx+1)*7 == val);
 * }
 * ```
 *
 * This code comes from this very useful blog entry.
 * https://www.reedbeta.com/blog/python-like-enumerate-in-cpp17/
 */
template <typename T,
          typename TIter = decltype(std::begin(std::declval<T>())),
          typename = decltype(std::end(std::declval<T>()))>
constexpr auto enumerate(T && iterable)
{
    struct iterator
    {
        size_t i;
        TIter iter;
        bool operator != (const iterator & other) const { return iter != other.iter; }
        void operator ++ () { ++i; ++iter; }
        auto operator * () const { return std::tie(i, *iter); }
    };
    struct iterable_wrapper
    {
        T iterable;
        auto begin() { return iterator{ 0, std::begin(iterable) }; }
        auto end() { return iterator{ 0, std::end(iterable) }; }
    };
    return iterable_wrapper{ std::forward<T>(iterable) };
}
}
}

#endif // SST_CPPUTILS_ITERATORS_H
