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

#ifndef INCLUDE_SST_CPPUTILS_BINDINGS_H
#define INCLUDE_SST_CPPUTILS_BINDINGS_H

#include <tuple>
#include <utility>

namespace sst
{
namespace cpputils
{
#ifndef DOXYGEN
namespace detail
{
template <typename Func, typename... FrontParams> class FrontBinder
{
    Func func;
    std::tuple<FrontParams...> frontArgsTuple;

  public:
    explicit FrontBinder(Func &&f, FrontParams &&...frontArgs)
        : func(std::move(f)), frontArgsTuple(std::move(frontArgs)...)
    {
    }

    template <typename... BackParams> auto operator()(BackParams &&...backArgs)
    {
        return std::apply(
            [this](auto... args) { return std::invoke(func, args...); },
            std::move(std::tuple_cat(frontArgsTuple, std::forward_as_tuple(backArgs...))));
    }

    template <typename... BackParams> auto operator()(BackParams &&...backArgs) const
    {
        return std::apply(
            [this](auto... args) { return std::invoke(func, args...); },
            std::move(std::tuple_cat(frontArgsTuple, std::forward_as_tuple(backArgs...))));
    }
};

template <typename Func, typename... BackParams> class BackBinder
{
    Func func;
    std::tuple<BackParams...> backArgsTuple;

  public:
    explicit BackBinder(Func &&f, BackParams &&...backArgs)
        : func(std::move(f)), backArgsTuple(std::move(backArgs)...)
    {
    }

    template <typename... FrontParams> auto operator()(FrontParams &&...frontArgs)
    {
        return std::apply(
            [this](auto... args) { return std::invoke(func, args...); },
            std::move(std::tuple_cat(std::forward_as_tuple(frontArgs...), backArgsTuple)));
    }

    template <typename... FrontParams> auto operator()(FrontParams &&...frontArgs) const
    {
        return std::apply(
            [this](auto... args) { return std::invoke(func, args...); },
            std::move(std::tuple_cat(std::forward_as_tuple(frontArgs...), backArgsTuple)));
    }
};
} // namespace detail
#endif // DOXYGEN

#ifdef __cpp_lib_bind_front
using std::bind_front;
#else
/** Temporary replacement for std::bind_front, which is only available in C++20 */
template <typename Func, typename... Params> auto bind_front(Func &&func, Params &&...frontParams)
{
    return detail::FrontBinder<Func, Params...>{std::forward<Func>(func),
                                                std::forward<Params>(frontParams)...};
}
#endif

#ifdef __cpp_lib_bind_back
using std::bind_back;
#else
/** Temporary replacement for std::bind_back, which is only available in C++23 */
template <typename Func, typename... Params> auto bind_back(Func &&func, Params &&...backParams)
{
    return detail::BackBinder<Func, Params...>{std::forward<Func>(func),
                                               std::forward<Params>(backParams)...};
}
#endif
} // namespace cpputils
} // namespace sst

#endif // SST_CPPUTILS_BINDINGS_H
