/*
 * sst-cpputils - an open source library of things we needed in C++
 * built by Surge Synth Team.
 *
 * Provides a collection of tools useful for writing C++-17 code
 *
 * Copyright 2022-2026, various authors, as described in the GitHub
 * transaction log.
 *
 * sst-cpputils is released under the MIT License found in the "LICENSE"
 * file in the root of this repository
 *
 * All source in sst-cpputils available at
 * https://github.com/surge-synthesizer/sst-cpputils
 */

#ifndef INCLUDE_SST_CPPUTILS_SMALL_HASH_MAP_H
#define INCLUDE_SST_CPPUTILS_SMALL_HASH_MAP_H

#include <array>
#include <cstddef>
#include <utility>
#include <functional>
#include <type_traits>
#include <cassert>

namespace sst::cpputils
{

/*
 * Small-buffer-optimized open-addressing hash containers for the realtime path.
 *
 * They hold up to InlineCap elements without touching the heap; past that they spill
 * to one heap allocation and *retain* that capacity across clear(), so a given instance
 * allocates at most once (on first exceeding its inline buffer) and is then allocation
 * free in steady state. Linear probing with power-of-two buckets; erase uses backward
 * shift deletion so there are no tombstones. The key must be hashable and equality
 * comparable. There is no per-element shrink.
 */

namespace smallhash_detail
{
constexpr size_t nextPow2(size_t n)
{
    size_t c{1};
    while (c < n)
        c <<= 1;
    return c;
}
// buckets sized so InlineCap elements stay under a 0.75 load factor
constexpr size_t bucketsFor(size_t inlineCap)
{
    return nextPow2((inlineCap == 0 ? 1 : inlineCap) * 4 / 3 + 1);
}
} // namespace smallhash_detail

template <typename K, typename V, size_t InlineCap, typename Hash = std::hash<K>> class SmallHashMap
{
    static constexpr size_t kBuckets = smallhash_detail::bucketsFor(InlineCap);

    struct Slot
    {
        std::pair<K, V> kv{};
        bool used{false};
    };

    Slot inlineSlots_[kBuckets];
    Slot *slots_{inlineSlots_};
    size_t cap_{kBuckets};
    size_t size_{0};

    bool spilled() const { return slots_ != inlineSlots_; }

    // is k cyclically within the open-left/closed-right interval (lo, hi]?
    static bool inCyclicRange(size_t k, size_t lo, size_t hi)
    {
        return (lo < hi) ? (lo < k && k <= hi) : (lo < k || k <= hi);
    }

    void rawInsert(K k, V v) // no load-factor check; used while rehashing
    {
        size_t mask = cap_ - 1;
        size_t i = Hash{}(k)&mask;
        while (slots_[i].used)
            i = (i + 1) & mask;
        slots_[i].kv = {std::move(k), std::move(v)};
        slots_[i].used = true;
        ++size_;
    }

    void grow(size_t want)
    {
        size_t newCap = smallhash_detail::nextPow2(want < kBuckets ? kBuckets : want);
        Slot *old = slots_;
        size_t oldCap = cap_;
        slots_ = new Slot[newCap]; // the rare, retained allocation
        cap_ = newCap;
        size_ = 0;
        for (size_t i = 0; i < oldCap; ++i)
            if (old[i].used)
                rawInsert(std::move(old[i].kv.first), std::move(old[i].kv.second));
        if (old != inlineSlots_)
            delete[] old;
    }

    void releaseHeap()
    {
        if (spilled())
            delete[] slots_;
        slots_ = inlineSlots_;
        cap_ = kBuckets;
        size_ = 0;
        for (auto &s : inlineSlots_)
            s.used = false;
    }

    void copyFrom(const SmallHashMap &o)
    {
        if (o.spilled())
        {
            slots_ = new Slot[o.cap_];
            cap_ = o.cap_;
        }
        else
        {
            slots_ = inlineSlots_;
            cap_ = kBuckets;
        }
        for (size_t i = 0; i < cap_; ++i)
            slots_[i] = o.slots_[i];
        size_ = o.size_;
    }

    void moveFrom(SmallHashMap &&o)
    {
        if (o.spilled())
        {
            slots_ = o.slots_; // steal the heap buffer
            cap_ = o.cap_;
            size_ = o.size_;
            o.slots_ = o.inlineSlots_; // can't steal an inline pointer; leave o empty-inline
            o.cap_ = kBuckets;
            o.size_ = 0;
            for (auto &s : o.inlineSlots_)
                s.used = false;
        }
        else
        {
            slots_ = inlineSlots_;
            cap_ = kBuckets;
            for (size_t i = 0; i < kBuckets; ++i)
                inlineSlots_[i] = std::move(o.inlineSlots_[i]);
            size_ = o.size_;
        }
    }

    void eraseIndex(size_t i)
    {
        const size_t mask = cap_ - 1;
        size_t j = i;
        while (true)
        {
            j = (j + 1) & mask;
            if (!slots_[j].used)
                break;
            size_t k = Hash{}(slots_[j].kv.first) & mask;
            if (!inCyclicRange(k, i, j)) // slots_[j] may fill the hole at i
            {
                slots_[i].kv = std::move(slots_[j].kv);
                slots_[i].used = true;
                i = j;
            }
        }
        slots_[i].used = false;
        --size_;
    }

  public:
    SmallHashMap() = default;
    ~SmallHashMap()
    {
        if (spilled())
            delete[] slots_;
    }
    SmallHashMap(const SmallHashMap &o) { copyFrom(o); }
    SmallHashMap(SmallHashMap &&o) noexcept { moveFrom(std::move(o)); }
    SmallHashMap &operator=(const SmallHashMap &o)
    {
        if (this != &o)
        {
            releaseHeap();
            copyFrom(o);
        }
        return *this;
    }
    SmallHashMap &operator=(SmallHashMap &&o) noexcept
    {
        if (this != &o)
        {
            releaseHeap();
            moveFrom(std::move(o));
        }
        return *this;
    }

    bool empty() const { return size_ == 0; }
    size_t size() const { return size_; }
    // Current bucket count. Starts at the inline capacity and only exceeds it once the container
    // has spilled to the heap, so capacity() growing past its initial value proves a spill.
    size_t capacity() const { return cap_; }

    template <bool Const> struct iter_t
    {
        using slot_t = std::conditional_t<Const, const Slot, Slot>;
        using pair_t = std::conditional_t<Const, const std::pair<K, V>, std::pair<K, V>>;
        slot_t *p, *e;
        void skip()
        {
            while (p != e && !p->used)
                ++p;
        }
        iter_t &operator++()
        {
            ++p;
            skip();
            return *this;
        }
        pair_t &operator*() const { return p->kv; }
        pair_t *operator->() const { return &p->kv; }
        bool operator==(const iter_t &o) const { return p == o.p; }
        bool operator!=(const iter_t &o) const { return p != o.p; }
    };
    using iterator = iter_t<false>;
    using const_iterator = iter_t<true>;

    iterator begin()
    {
        iterator it{slots_, slots_ + cap_};
        it.skip();
        return it;
    }
    iterator end() { return {slots_ + cap_, slots_ + cap_}; }
    const_iterator begin() const
    {
        const_iterator it{slots_, slots_ + cap_};
        it.skip();
        return it;
    }
    const_iterator end() const { return {slots_ + cap_, slots_ + cap_}; }

    iterator find(const K &k)
    {
        size_t mask = cap_ - 1;
        for (size_t i = Hash{}(k)&mask; slots_[i].used; i = (i + 1) & mask)
            if (slots_[i].kv.first == k)
                return {slots_ + i, slots_ + cap_};
        return end();
    }

    const_iterator find(const K &k) const
    {
        size_t mask = cap_ - 1;
        for (size_t i = Hash{}(k)&mask; slots_[i].used; i = (i + 1) & mask)
            if (slots_[i].kv.first == k)
                return {slots_ + i, slots_ + cap_};
        return end();
    }

    V &operator[](const K &k)
    {
        if (4 * (size_ + 1) > 3 * cap_)
            grow(cap_ * 2);
        size_t mask = cap_ - 1;
        size_t i = Hash{}(k)&mask;
        while (slots_[i].used)
        {
            if (slots_[i].kv.first == k)
                return slots_[i].kv.second;
            i = (i + 1) & mask;
        }
        slots_[i].kv = {k, V{}};
        slots_[i].used = true;
        ++size_;
        return slots_[i].kv.second;
    }

    V &at(const K &k)
    {
        auto it = find(k);
        assert(it != end());
        return it->second;
    }

    const V &at(const K &k) const
    {
        auto it = find(k);
        assert(it != end());
        return it->second;
    }

    void erase(iterator it)
    {
        if (it != end())
            eraseIndex(static_cast<size_t>(it.p - slots_));
    }
    void erase(const K &k) { erase(find(k)); }

    void clear()
    {
        for (size_t i = 0; i < cap_; ++i)
            slots_[i].used = false;
        size_ = 0;
    }
};

template <typename K, size_t InlineCap, typename Hash = std::hash<K>> class SmallHashSet
{
    static constexpr size_t kBuckets = smallhash_detail::bucketsFor(InlineCap);

    struct Slot
    {
        K key{};
        bool used{false};
    };

    Slot inlineSlots_[kBuckets];
    Slot *slots_{inlineSlots_};
    size_t cap_{kBuckets};
    size_t size_{0};

    bool spilled() const { return slots_ != inlineSlots_; }

    void rawInsert(K k)
    {
        size_t mask = cap_ - 1;
        size_t i = Hash{}(k)&mask;
        while (slots_[i].used)
            i = (i + 1) & mask;
        slots_[i].key = std::move(k);
        slots_[i].used = true;
        ++size_;
    }

    void grow(size_t want)
    {
        size_t newCap = smallhash_detail::nextPow2(want < kBuckets ? kBuckets : want);
        Slot *old = slots_;
        size_t oldCap = cap_;
        slots_ = new Slot[newCap];
        cap_ = newCap;
        size_ = 0;
        for (size_t i = 0; i < oldCap; ++i)
            if (old[i].used)
                rawInsert(std::move(old[i].key));
        if (old != inlineSlots_)
            delete[] old;
    }

    void releaseHeap()
    {
        if (spilled())
            delete[] slots_;
        slots_ = inlineSlots_;
        cap_ = kBuckets;
        size_ = 0;
        for (auto &s : inlineSlots_)
            s.used = false;
    }

    void copyFrom(const SmallHashSet &o)
    {
        if (o.spilled())
        {
            slots_ = new Slot[o.cap_];
            cap_ = o.cap_;
        }
        else
        {
            slots_ = inlineSlots_;
            cap_ = kBuckets;
        }
        for (size_t i = 0; i < cap_; ++i)
            slots_[i] = o.slots_[i];
        size_ = o.size_;
    }

    void moveFrom(SmallHashSet &&o)
    {
        if (o.spilled())
        {
            slots_ = o.slots_;
            cap_ = o.cap_;
            size_ = o.size_;
            o.slots_ = o.inlineSlots_;
            o.cap_ = kBuckets;
            o.size_ = 0;
            for (auto &s : o.inlineSlots_)
                s.used = false;
        }
        else
        {
            slots_ = inlineSlots_;
            cap_ = kBuckets;
            for (size_t i = 0; i < kBuckets; ++i)
                inlineSlots_[i] = std::move(o.inlineSlots_[i]);
            size_ = o.size_;
        }
    }

  public:
    SmallHashSet() = default;
    ~SmallHashSet()
    {
        if (spilled())
            delete[] slots_;
    }
    SmallHashSet(const SmallHashSet &o) { copyFrom(o); }
    SmallHashSet(SmallHashSet &&o) noexcept { moveFrom(std::move(o)); }
    SmallHashSet &operator=(const SmallHashSet &o)
    {
        if (this != &o)
        {
            releaseHeap();
            copyFrom(o);
        }
        return *this;
    }
    SmallHashSet &operator=(SmallHashSet &&o) noexcept
    {
        if (this != &o)
        {
            releaseHeap();
            moveFrom(std::move(o));
        }
        return *this;
    }

    bool empty() const { return size_ == 0; }
    size_t size() const { return size_; }
    // Current bucket count. Starts at the inline capacity and only exceeds it once the container
    // has spilled to the heap, so capacity() growing past its initial value proves a spill.
    size_t capacity() const { return cap_; }

    bool contains(const K &k) const
    {
        size_t mask = cap_ - 1;
        for (size_t i = Hash{}(k)&mask; slots_[i].used; i = (i + 1) & mask)
            if (slots_[i].key == k)
                return true;
        return false;
    }

    void insert(const K &k)
    {
        if (contains(k))
            return;
        if (4 * (size_ + 1) > 3 * cap_)
            grow(cap_ * 2);
        rawInsert(k);
    }

    void clear()
    {
        for (size_t i = 0; i < cap_; ++i)
            slots_[i].used = false;
        size_ = 0;
    }

    template <bool Const> struct iter_t
    {
        using slot_t = std::conditional_t<Const, const Slot, Slot>;
        using key_t = std::conditional_t<Const, const K, K>;
        slot_t *p, *e;
        void skip()
        {
            while (p != e && !p->used)
                ++p;
        }
        iter_t &operator++()
        {
            ++p;
            skip();
            return *this;
        }
        key_t &operator*() const { return p->key; }
        bool operator==(const iter_t &o) const { return p == o.p; }
        bool operator!=(const iter_t &o) const { return p != o.p; }
    };
    using iterator = iter_t<false>;
    using const_iterator = iter_t<true>;

    iterator begin()
    {
        iterator it{slots_, slots_ + cap_};
        it.skip();
        return it;
    }
    iterator end() { return {slots_ + cap_, slots_ + cap_}; }
    const_iterator begin() const
    {
        const_iterator it{slots_, slots_ + cap_};
        it.skip();
        return it;
    }
    const_iterator end() const { return {slots_ + cap_, slots_ + cap_}; }
};

} // namespace sst::cpputils

#endif // INCLUDE_SST_CPPUTILS_SMALL_HASH_MAP_H
