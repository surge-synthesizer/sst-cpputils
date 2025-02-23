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

#ifndef INCLUDE_SST_CPPUTILS_FIXED_ALLOCATER_H
#define INCLUDE_SST_CPPUTILS_FIXED_ALLOCATER_H

#include <cstdint>
#include <iostream>
#include <cstring>

namespace sst::cpputils
{
/**
 * template<typename T, int N> STLFixedAllocator provides
 * an implementation of the stl Allocator protocol on a
 * fixed array of size N. A common use would be to have T
 * be an int or a pointer. It basically works as a shared pointer.
 *
 * Note this allocator is *not* thread safe. do not use it
 * for STL classes which are shared across multiple threads.
 *
 * As of this writing you should *not* use this class. It works
 * but the allocation method is painfully naive. It exists as
 * an interface outline for how you would allocate in STL onto
 * pre-allocated blocks and nothing more (and it works, but has
 * a linear-time-in-N allocate cost).
 */
template <typename T, int N> struct fixed_memory_allocator_impl
{
    /**
     * Don't allow a default constructor so the allocation has to happen elsewhere
     * and this needs to be explicitly passed to the STL container
     *
     * @param allocate always set this to true. Its just here to
     * make a constructor not default
     */
    explicit fixed_memory_allocator_impl(bool allocate)
    {
        if (allocate)
        {
            std::cout << "allocating block " << this << std::endl;
            dataOwner_ = allocate;
            data_ = new std::uint8_t[N];
            used_ = new bool[N];
            refCount_ = new size_t;
            memset(used_, 0, N);
            *refCount_ = 1;
        }
        else
        {
            throw std::bad_alloc();
        }
    }

    // This is the constructor called by rebind
    template <typename U>
    fixed_memory_allocator_impl(const fixed_memory_allocator_impl<U, N> &other)
    {
        dataOwner_ = false;
        data_ = other.data_;
        used_ = other.used_;
        refCount_ = other.refCount_;
        *refCount_ += 1;
    }

    fixed_memory_allocator_impl(const fixed_memory_allocator_impl &other)
    {
        dataOwner_ = false;
        data_ = other.data_;
        used_ = other.used_;
        refCount_ = other.refCount_;
        *refCount_ += 1;
    }

    fixed_memory_allocator_impl(const fixed_memory_allocator_impl &&other) noexcept
    {
        dataOwner_ = false;
        data_ = other.data_;
        used_ = other.used_;
        refCount_ = other.refCount_;
        *refCount_ += 1;
    }

    fixed_memory_allocator_impl &operator=(const fixed_memory_allocator_impl &other)
    {
        if (this == &other)
        {
            return *this;
        }
        dataOwner_ = false;
        data_ = other.data_;
        used_ = other.used_;
        refCount_ = other.refCount_;
        other.dataOwner_ = false;
        other.data_ = nullptr;
        other.used_ = nullptr;
        other.refCount_ = nullptr;
        return *this;
    }

    fixed_memory_allocator_impl &operator=(fixed_memory_allocator_impl &&other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        dataOwner_ = other.dataOwner_;
        data_ = other.data_;
        used_ = other.used_;
        refCount_ = other.refCount_;
        other.dataOwner_ = false;
        other.data_ = nullptr;
        other.used_ = nullptr;
        other.refCount_ = nullptr;
        return *this;
    }

    ~fixed_memory_allocator_impl()
    {
        *refCount_ -= 1;
        if (*refCount_ == 0)
        {
#ifndef NDEBUG
            for (auto i = 0U; i < N; ++i)
            {
                assert(!used_[i]);
            }
#endif
            // std::cout << "deallocating block " << this << std::endl;
            delete[] data_;
            delete[] used_;
            delete[] refCount_;
        }
    }

    using value_type = T;

    /**
     * This allocator is pretty naive way of searching for empty blocks
     * but its functional and non-allocating.
     *
     * @param n
     * @return
     */
    value_type *allocate(std::size_t n)
    {
        auto sot = sizeof(T);
        if (n * sot > N)
        {
            throw std::bad_alloc(); // Cannot allocate more than N objects
        }

        auto blockSize = n * sot;
        // Find a contiguous block of available memory
        for (std::size_t i = 0; i <= N - blockSize; ++i)
        {
            bool free_block = true;
            for (std::size_t j = 0; j < blockSize; ++j)
            {
                if (used_[i + j])
                {
                    // We can advance through the allocated region
                    while (j < N - blockSize && used_[i + j])
                        ++j;
                    if (j > 0)
                        i = i + j - 1; // because the loop will +1 it
                    free_block = false;
                    break;
                }
            }
            if (free_block)
            {
                assert(!used_[i]);
                for (std::size_t j = 0; j < blockSize; ++j)
                {
                    used_[i + j] = true;
                }
                // std::cout << "allocate " << n << " * " << sot << " in " << this << " at " << i <<
                // std::endl; debugDumpUsed();
                return reinterpret_cast<T *>(&data_[i]);
            }
        }

        throw std::bad_alloc(); // No available block found
    }

    void debugDumpUsed()
    {
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 32; ++j)
            {
                std::cout << (used_[i * 16 + j] ? "+" : ".");
                if (j % 8 == 7)
                    std::cout << " ";
            }
            std::cout << std::endl;
        }
    }

    void deallocate(value_type *p, std::size_t n) noexcept
    {
        auto sot = sizeof(T);
        auto blockSize = n * sot;

        auto offset = reinterpret_cast<std::uint8_t *>(p) - data_;
        auto start_index = offset;
        for (std::size_t i = 0; i < blockSize; ++i)
        {
            used_[start_index + i] = false;
        }
        // std::cout << "deallocate " << n << " * " << sot << " in " << this << " at " << offset <<
        // std::endl; debugDumpUsed();
    }

    template <typename U> struct rebind
    {
        using other = fixed_memory_allocator_impl<U, N>;
    };

    std::uint8_t *data_{nullptr};
    bool *used_{nullptr}; // Track usage of buffer slots
    size_t *refCount_{nullptr};
    bool dataOwner_{false};
};

template <typename T, int N>
using fixed_memory_allocator =
    typename std::allocator_traits<fixed_memory_allocator_impl<T, N>>::template rebind_alloc<T>;
} // namespace sst::cpputils

#endif // STLFIXEDALLOCATOR_H
