/*
 * sst-cpputils - an open source library of things we needed in C++
 * built by Surge Synth Team.
 *
 * Provides a collection of tools useful for writing C++-17 code
 *
 * Copyright 2022-2025, various authors, as described in the GitHub
 * transaction log.
 *
 * sst-cpputils is released under the MIT License found in the "LICENSE"
 * file in the root of this repository
 *
 * All source in sst-cpputils available at
 * https://github.com/surge-synthesizer/sst-cpputils
 */

#ifndef INCLUDE_SST_CPPUTILS_ALIGNED_ALLOCATOR_H
#define INCLUDE_SST_CPPUTILS_ALIGNED_ALLOCATOR_H

#include <memory>

namespace sst::cpputils
{

// Aligned allocator.
// Adapted from the Seqan3 library, licensed under BSD 3-clause.
// Copyright (c) 2006-2022, Knut Reinert & Freie Universität Berlin
// Copyright (c) 2016-2022, Knut Reinert & MPI für molekulare Genetik
// Copyright (c) 2025, Surge Synth Team
template <typename value_t, std::size_t alignment_v = __STDCPP_DEFAULT_NEW_ALIGNMENT__>
class AlignedAllocator
{
  public:
    static constexpr std::size_t alignment = alignment_v;

    using value_type = value_t;
    using pointer = value_type *;
    using difference_type = typename std::pointer_traits<pointer>::difference_type;
    using size_type = std::make_unsigned_t<difference_type>;

    using is_always_equal = std::true_type;

    AlignedAllocator() = default;
    AlignedAllocator(AlignedAllocator const &) = default;
    AlignedAllocator(AlignedAllocator &&) = default;
    AlignedAllocator &operator=(AlignedAllocator const &) = default;
    AlignedAllocator &operator=(AlignedAllocator &&) = default;
    ~AlignedAllocator() = default;

    template <class other_value_type, std::size_t other_alignment>
    constexpr AlignedAllocator(AlignedAllocator<other_value_type, other_alignment> const &) noexcept
    {
    }

    [[nodiscard]] pointer allocate(size_type const n) const
    {
        constexpr size_type max_size = std::numeric_limits<size_type>::max() / sizeof(value_type);
        if (n > max_size)
            throw std::bad_alloc{};

        std::size_t bytes_to_allocate = n * sizeof(value_type);
        if constexpr (alignment <= __STDCPP_DEFAULT_NEW_ALIGNMENT__)
            return static_cast<pointer>(::operator new(bytes_to_allocate));
        else // Use alignment aware allocator function.
            return static_cast<pointer>(
                ::operator new(bytes_to_allocate, static_cast<std::align_val_t>(alignment)));
    }

    void deallocate(pointer const p, size_type const n) const noexcept
    {
        std::size_t bytes_to_deallocate = n * sizeof(value_type);

        if constexpr (alignment <= __STDCPP_DEFAULT_NEW_ALIGNMENT__)
            ::operator delete(p, bytes_to_deallocate);
        else // Use alignment aware deallocator function.
            ::operator delete(p, bytes_to_deallocate, static_cast<std::align_val_t>(alignment));
    }

    template <class value_type2, std::size_t alignment2>
    constexpr bool operator==(AlignedAllocator<value_type2, alignment2> const &) noexcept
    {
        return alignment == alignment2;
    }

    template <class value_type2, std::size_t alignment2>
    constexpr bool operator!=(AlignedAllocator<value_type2, alignment2> const &) noexcept
    {
        return alignment != alignment2;
    }
};

} // namespace sst::cpputils

#endif // INCLUDE_SST_CPPUTILS_ALLIGNEDALLOCATOR_H
