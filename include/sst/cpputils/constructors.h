/*
 * Constructors - ways to make elements, to date really a way
 * to make an array which contains elements without a default
 * constructor
 */

#ifndef SST_CPPUTILS_CONSTRUCTORS_H
#define SST_CPPUTILS_CONSTRUCTORS_H

#include <array>

namespace sst::cpputils
{
namespace detail
{
template <typename T, size_t... Is, typename... Args>
std::array<T, sizeof...(Is)> make_array_helper(std::index_sequence<Is...>, Args &&...args)
{
    return {(static_cast<void>(Is), T{std::forward<Args>(args)...})...};
}
} // namespace detail

template <typename T, size_t N, typename... Args>
std::array<T, N> make_array(Args&&... args) {
    return detail::make_array_helper<T>(std::make_index_sequence<N>{},
                              std::forward<Args>(args)...);
}
}

#endif // CONDUIT_CONSTRUCTORS_H
