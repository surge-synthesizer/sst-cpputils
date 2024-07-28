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

#ifndef INCLUDE_SST_CPPUTILS_CONSTRUCTORS_H
#define INCLUDE_SST_CPPUTILS_CONSTRUCTORS_H

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

template <typename T, size_t... Is, typename... Args>
std::array<T, sizeof...(Is)> make_array_helper_last_index(std::index_sequence<Is...>,
                                                          Args &&...args)
{
    return {(static_cast<void>(Is), T{std::forward<Args>(args)..., Is})...};
}

template <typename T, size_t... Is, typename... Args>
std::array<T, sizeof...(Is)> make_array_helper_first_index(std::index_sequence<Is...>,
                                                           Args &&...args)
{
    return {(static_cast<void>(Is), T{Is, std::forward<Args>(args)...})...};
}

template <typename T, size_t... Is, typename Maker>
std::array<T, sizeof...(Is)> make_array_lambda(std::index_sequence<Is...>, Maker &&maker)
{
    return {T{maker(std::integral_constant<size_t, Is>())}...};
}
} // namespace detail

/*
 * Construct an n element array for a type which is non-default constructable. So
 *
 * struct Foo { Foo(int a, const std::string &b) {} };
 * std::array<Foo,18> foo;
 *
 * has a problem when constructing. You can do
 *
 * std::array<Foo, 18> foo{sst::cpputils::make_array<Foo, 18>(174, "test")};
 *
 * and get expected result. We also have two variations which can bind the first
 * or last constructor element to th eindex so
 *
 * std::array<Foo, 18> foo{sst::cpputils::make_array_bind_first_idnex<Foo, 18>("test")};
 *
 * will construct with the a bound to 0, 1, 2, ...
 */
template <typename T, size_t N, typename... Args> std::array<T, N> make_array(Args &&...args)
{
    return detail::make_array_helper<T>(std::make_index_sequence<N>{}, std::forward<Args>(args)...);
}

template <typename T, size_t N, typename... Args>
std::array<T, N> make_array_bind_last_index(Args &&...args)
{
    return detail::make_array_helper_last_index<T>(std::make_index_sequence<N>{},
                                                   std::forward<Args>(args)...);
}

template <typename T, size_t N, typename... Args>
std::array<T, N> make_array_bind_first_index(Args &&...args)
{
    return detail::make_array_helper_first_index<T>(std::make_index_sequence<N>{},
                                                    std::forward<Args>(args)...);
}

/** Returns an array of size N, with each value initialized by calling the maker lambda with
 * signature [] (size_t index) -> T. */
template <typename T, size_t N, typename Maker>
constexpr std::array<T, N> make_array_lambda(Maker &&maker)
{
    return detail::make_array_lambda<T>(std::make_index_sequence<N>{}, std::forward<Maker>(maker));
}
} // namespace sst::cpputils

#endif // SST_CPPUTILS_CONSTRUCTORS_H
