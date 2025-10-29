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
#ifndef INCLUDE_SST_CPPUTILS_DYN_ARRAY_H
#define INCLUDE_SST_CPPUTILS_DYN_ARRAY_H

#include <initializer_list>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>

namespace sst
{
namespace cpputils
{

// DynArray: an array where the size is defined at construction time, instead
// of compile time (unlike std::array) and is constant at construction (unlike
// std::vector). Has (mostly) the same interface as std::vector.
template <typename T, class Allocator = std::allocator<T>> class DynArray
{
  public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::allocator_traits<Allocator>::size_type;
    using difference_type = std::allocator_traits<Allocator>::difference_type;
    using reference = T &;
    using const_reference = const T &;
    using pointer = std::allocator_traits<Allocator>::pointer;
    using const_pointer = std::allocator_traits<Allocator>::const_pointer;
    using iterator = pointer;
    using move_iterator = std::move_iterator<iterator>;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    //
    // Creates an array that stores "n" elements.
    // Initializes them with the provided arguments.
    template <typename... ConstructionArgs>
    DynArray(std::size_t n, Allocator a, ConstructionArgs &&...args);

    template <typename... ConstructionArgs>
    explicit DynArray(std::size_t n, ConstructionArgs &&...args)
        : DynArray(n, Allocator(), std::forward<ConstructionArgs>(args)...)
    {
    }

    //
    // Creates an array with "n" copies of "v"
    DynArray(std::size_t n, const T &v, Allocator a = Allocator())
        requires(std::is_copy_constructible_v<T>);

    //
    // Creates an array initialized with the size and contents of the init_list.
    DynArray(std::initializer_list<T> init_list, Allocator a = Allocator());

    //
    // Copy constructors and assignment.
    DynArray(const DynArray &that)
        requires(std::is_copy_constructible_v<T>);
    DynArray &operator=(const DynArray &that)
        requires(std::is_copy_constructible_v<T>);
    DynArray(const DynArray &that, Allocator a)
        requires(std::is_copy_constructible_v<T>)
        : DynArray(that.begin(), that.end(), a)
    {
    }
    template <std::forward_iterator Iterator>
    DynArray(Iterator first, Iterator last, Allocator a = Allocator());

    //
    // Move constructor and assignment.
    // This will propagate (*copy*) the allocator.
    DynArray(DynArray &&that);
    DynArray &operator=(DynArray &&that);

    ~DynArray();

    //
    // The usual C++ container iterators and accessors
    iterator begin() { return mem_; }
    const_iterator begin() const { return mem_; }
    const_iterator cbegin() const { return begin(); }

    iterator end() { return mem_ + n_; }
    const_iterator end() const { return mem_ + n_; }
    const_iterator cend() const { return end(); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const { return rbegin(); }

    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const { return rend(); }

    pointer data() { return mem_; }
    const_pointer data() const { return mem_; }

    reference operator[](size_type i);
    const_reference operator[](size_type i) const;

    reference at(size_type i);
    const_reference at(size_type i) const;

    reference front();
    const_reference front() const;

    reference back();
    const_reference back() const;

    //
    // The usual C++ container utilities
    void fill(const value_type &val) { std::fill(begin(), end(), val); }

    std::size_t size() const { return n_; }
    bool empty() const { return n_ == 0; }

    //
    // Post-construction changes. Destroys all existing contents.
    void reset(std::size_t n);

    //
    // Elementwise equality.
    friend bool operator==(const DynArray &lhs, const DynArray &rhs)
    {
        return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    friend bool operator!=(const DynArray &lhs, const DynArray &rhs) { return !(lhs == rhs); }

  private:
    inline void check_range(size_type i);
    void deallocate();

    Allocator alloc_;
    T *mem_;
    std::size_t n_;
};

template <typename T, class A>
template <typename... ConstructionArgs>
DynArray<T, A>::DynArray(std::size_t n, A a, ConstructionArgs &&...args)
    : alloc_(std::move(a)), mem_(nullptr), n_(n)
{
    mem_ = alloc_.allocate(n);
    for (std::size_t i = 0; i < n; i++)
    {
        std::allocator_traits<A>::construct(alloc_, mem_ + i,
                                            std::forward<ConstructionArgs>(args)...);
    }
}

template <typename T, class A>
DynArray<T, A>::DynArray(const DynArray<T, A> &that)
    requires(std::is_copy_constructible_v<T>)
    : DynArray<T, A>(that,
                     std::allocator_traits<A>::select_on_container_copy_construction(that.alloc_))
{
}

template <typename T, class A>
DynArray<T, A> &DynArray<T, A>::operator=(const DynArray<T, A> &that)
    requires(std::is_copy_constructible_v<T>)
{
    deallocate();
    mem_ = that.mem_;
    alloc_ = std::allocator_traits<A>::select_on_container_copy_construction(that.alloc_);
    n_ = that.n_;
    mem_ = alloc_.allocate(n_);
    std::uninitialized_copy(that.begin(), that.end(), mem_);
    return *this;
}

template <typename T, class A>
DynArray<T, A>::DynArray(std::size_t n, const T &v, A a)
    requires(std::is_copy_constructible_v<T>)
    : alloc_(std::move(a)), mem_(nullptr), n_(n)
{
    mem_ = alloc_.allocate(n);
    std::uninitialized_fill(begin(), end(), v);
}

template <typename T, class A>
DynArray<T, A>::DynArray(std::initializer_list<T> init_list, A a)
    : mem_(nullptr), n_(init_list.size()), alloc_(std::move(a))
{
    mem_ = alloc_.allocate(n_);
    std::uninitialized_move(std::make_move_iterator(init_list.begin()),
                            std::make_move_iterator(init_list.end()), mem_);
}

template <typename T, class A>
template <std::forward_iterator Iterator>
DynArray<T, A>::DynArray(Iterator begin, Iterator end, A a)
    : alloc_(a), mem_(nullptr), n_(std::distance(begin, end))
{
    mem_ = alloc_.allocate(n_);
    try
    {
        std::uninitialized_copy(begin, end, mem_);
    }
    catch (...)
    {
        alloc_.deallocate(mem_, n_);
        throw;
    }
}

template <typename T, class A> DynArray<T, A>::DynArray(DynArray<T, A> &&that) : alloc_(that.alloc_)
{
    mem_ = that.mem_;
    that.mem_ = nullptr;
    n_ = that.n_;
    that.n_ = 0;
}

template <typename T, class A> DynArray<T, A> &DynArray<T, A>::operator=(DynArray<T, A> &&that)
{
    deallocate();

    alloc_ = that.alloc_;
    mem_ = that.mem_;
    that.mem_ = nullptr;
    n_ = that.n_;
    that.n_ = 0;
    return *this;
}

template <typename T, class A> DynArray<T, A>::~DynArray() { deallocate(); }

template <typename T, class A>
DynArray<T, A>::reference DynArray<T, A>::operator[](DynArray<T, A>::size_type i)
{
    check_range(i);
    return mem_[i];
}

template <typename T, class A>
DynArray<T, A>::const_reference DynArray<T, A>::operator[](DynArray<T, A>::size_type i) const
{
    check_range(i);
    return mem_[i];
}

template <typename T, class A>
DynArray<T, A>::reference DynArray<T, A>::at(DynArray<T, A>::size_type i)
{
    check_range(i);
    return mem_[i];
}

template <typename T, class A>
DynArray<T, A>::const_reference DynArray<T, A>::at(DynArray<T, A>::size_type i) const
{
    check_range(i);
    return mem_[i];
}

template <typename T, class A> DynArray<T, A>::reference DynArray<T, A>::front()
{
    if (empty()) [[unlikely]]
        throw std::domain_error("uninitialized DynArray");
    return mem_[0];
}

template <typename T, class A> DynArray<T, A>::const_reference DynArray<T, A>::front() const
{
    if (empty()) [[unlikely]]
        throw std::domain_error("uninitialized DynArray");
    return mem_[0];
}

template <typename T, class A> DynArray<T, A>::reference DynArray<T, A>::back()
{
    if (empty()) [[unlikely]]
        throw std::domain_error("uninitialized DynArray");
    return mem_[n_ - 1];
}

template <typename T, class A> DynArray<T, A>::const_reference DynArray<T, A>::back() const
{
    if (empty()) [[unlikely]]
        throw std::domain_error("uninitialized DynArray");
    return mem_[n_ - 1];
}

template <typename T, class A> void DynArray<T, A>::reset(std::size_t n)
{
    deallocate();
    n_ = n;
    mem_ = alloc_.allocate(n_);
    for (std::size_t i = 0; i < n; i++)
    {
        std::allocator_traits<A>::construct(alloc_, mem_ + i);
    }
}

template <typename T, class A> void DynArray<T, A>::check_range(DynArray<T, A>::size_type i)
{
    if (mem_ == nullptr) [[unlikely]]
        throw std::domain_error("uninitialized DynArray");
    if (i >= n_) [[unlikely]]
        throw std::out_of_range(std::string("elt ") + std::to_string(i) + ", size " +
                                std::to_string(n_));
}

template <typename T, class A> void DynArray<T, A>::deallocate()
{
    if (mem_ == nullptr)
        return;

    for (std::size_t i = 0; i < n_; i++)
    {
        std::allocator_traits<A>::destroy(alloc_, mem_ + i);
    }
    alloc_.deallocate(mem_, n_);
}

} // namespace cpputils
} // namespace sst

#endif // INCLUDE_SST_CPPUTILS_FIXED_ARRAY_H
