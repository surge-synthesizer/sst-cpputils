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

#include "sst/cpputils/small_hash_map.h"

#include "catch2.hpp"

#include <set>
#include <type_traits>
#include <utility>

using sst::cpputils::SmallHashMap;
using sst::cpputils::SmallHashSet;

TEST_CASE("SmallHashMap basic operations")
{
    SmallHashMap<int, int, 8> m;
    REQUIRE(m.empty());
    REQUIRE(m.size() == 0);

    m[1] = 10;
    m[2] = 20;
    m[3] = 30;
    REQUIRE_FALSE(m.empty());
    REQUIRE(m.size() == 3);
    REQUIRE(m[1] == 10);
    REQUIRE(m[2] == 20);
    REQUIRE(m.at(3) == 30);

    // operator[] on an existing key returns the slot and updates in place
    m[2] = 99;
    REQUIRE(m.size() == 3);
    REQUIRE(m[2] == 99);

    // find hits and misses
    REQUIRE(m.find(2) != m.end());
    REQUIRE(m.find(2)->second == 99);
    REQUIRE(m.find(12345) == m.end());

    m.clear();
    REQUIRE(m.empty());
    REQUIRE(m.size() == 0);
    REQUIRE(m.find(1) == m.end());
}

TEST_CASE("SmallHashMap iteration visits every entry exactly once")
{
    SmallHashMap<int, int, 8> m;
    for (int i = 0; i < 6; ++i)
        m[i] = i * 100;

    std::set<int> seen;
    int count = 0;
    for (auto &[k, v] : m)
    {
        REQUIRE(v == k * 100);
        seen.insert(k);
        ++count;
    }
    REQUIRE(count == 6);
    REQUIRE(seen.size() == 6);
}

TEST_CASE("SmallHashMap erase by key and iterator")
{
    SmallHashMap<int, int, 8> m;
    for (int i = 0; i < 6; ++i)
        m[i] = i;

    m.erase(2);
    REQUIRE(m.size() == 5);
    REQUIRE(m.find(2) == m.end());
    for (int i : {0, 1, 3, 4, 5})
    {
        REQUIRE(m.find(i) != m.end());
        REQUIRE(m.find(i)->second == i);
    }

    auto it = m.find(4);
    REQUIRE(it != m.end());
    m.erase(it);
    REQUIRE(m.size() == 4);
    REQUIRE(m.find(4) == m.end());

    // erasing an absent key is a no-op
    m.erase(999);
    REQUIRE(m.size() == 4);
}

// Hash everything to one bucket so every key shares a probe chain. This is the worst case for
// linear probing and exercises the backward-shift deletion that keeps the table tombstone-free.
struct AllCollide
{
    size_t operator()(int) const { return 0; }
};

TEST_CASE("SmallHashMap backward-shift deletion under total collision")
{
    SmallHashMap<int, int, 16, AllCollide> m;
    for (int i = 0; i < 10; ++i)
        m[i] = i * 7;
    REQUIRE(m.size() == 10);

    // Remove from the middle of the chain; every survivor must still be reachable.
    m.erase(5);
    REQUIRE(m.find(5) == m.end());
    for (int i = 0; i < 10; ++i)
        if (i != 5)
        {
            REQUIRE(m.find(i) != m.end());
            REQUIRE(m.find(i)->second == i * 7);
        }

    // Remove the head and tail of the chain and another interior element.
    for (int i : {0, 9, 3})
        m.erase(i);
    REQUIRE(m.size() == 6);
    for (int i : {1, 2, 4, 6, 7, 8})
    {
        REQUIRE(m.find(i) != m.end());
        REQUIRE(m.find(i)->second == i * 7);
    }
}

TEST_CASE("SmallHashMap spills to the heap past its inline capacity")
{
    SmallHashMap<int, int, 2> m; // deliberately tiny inline buffer
    const auto inlineBuckets = m.capacity();
    REQUIRE(inlineBuckets >= 2);

    // A handful of entries stay inline: capacity must not change (no heap allocation).
    m[0] = 0;
    REQUIRE(m.capacity() == inlineBuckets);

    // Push well past the inline buffer. This forces at least one grow()+rehash to the heap.
    constexpr int N = 200;
    for (int i = 0; i < N; ++i)
        m[i] = i * 3;

    REQUIRE(m.size() == N);
    REQUIRE(m.capacity() > inlineBuckets); // capacity grew beyond inline => it spilled to the heap

    // Every entry survived the grow/rehash with the right value.
    for (int i = 0; i < N; ++i)
    {
        REQUIRE(m.find(i) != m.end());
        REQUIRE(m.at(i) == i * 3);
    }

    // clear() keeps the grown capacity (so steady-state reuse never re-allocates) and the map
    // remains fully usable afterward.
    const auto grown = m.capacity();
    m.clear();
    REQUIRE(m.empty());
    REQUIRE(m.capacity() == grown);
    m[42] = 7;
    REQUIRE(m.size() == 1);
    REQUIRE(m[42] == 7);
}

TEST_CASE("SmallHashMap copy and move with a spilled buffer")
{
    SmallHashMap<int, int, 2> src;
    for (int i = 0; i < 50; ++i)
        src[i] = i + 1; // forces a spill
    REQUIRE(src.size() == 50);
    REQUIRE(src.capacity() > 4);

    // A copy is a fully independent deep copy.
    auto cp = src;
    REQUIRE(cp.size() == 50);
    cp[0] = 999;
    REQUIRE(cp[0] == 999);
    REQUIRE(src[0] == 1); // source untouched
    for (int i = 1; i < 50; ++i)
        REQUIRE(cp.at(i) == i + 1);

    // A move carries the contents across.
    auto mv = std::move(src);
    REQUIRE(mv.size() == 50);
    for (int i = 1; i < 50; ++i)
        REQUIRE(mv.at(i) == i + 1);
}

TEST_CASE("SmallHashSet basic operations, dedup, and spill")
{
    SmallHashSet<int, 4> s;
    REQUIRE(s.empty());

    s.insert(10);
    s.insert(20);
    s.insert(10); // duplicate is ignored
    REQUIRE(s.size() == 2);
    REQUIRE(s.contains(10));
    REQUIRE(s.contains(20));
    REQUIRE_FALSE(s.contains(30));

    // Spill: 10 and 20 are within 0..99 so the final count is exactly 100.
    const auto inlineBuckets = s.capacity();
    for (int i = 0; i < 100; ++i)
        s.insert(i);
    REQUIRE(s.size() == 100);
    REQUIRE(s.capacity() > inlineBuckets); // spilled to the heap
    for (int i = 0; i < 100; ++i)
        REQUIRE(s.contains(i));

    std::set<int> seen;
    for (auto k : s)
        seen.insert(k);
    REQUIRE(seen.size() == 100);

    s.clear();
    REQUIRE(s.empty());
    REQUIRE_FALSE(s.contains(5));
}

TEST_CASE("SmallHashMap const find and at (inline)")
{
    SmallHashMap<int, int, 8> m;
    for (int i = 0; i < 5; ++i)
        m[i] = i * 10;

    // Resolve find/at through a const reference: this picks the const-qualified overloads (the
    // non-const ones are not callable on a const object, so reaching them here proves they exist).
    const auto &cm = m;

    // at() const returns a reference to const
    static_assert(std::is_same_v<decltype(cm.at(0)), const int &>);
    REQUIRE(cm.at(0) == 0);
    REQUIRE(cm.at(4) == 40);

    // find() const yields a const_iterator with const access to the stored pair
    static_assert(std::is_same_v<decltype(cm.find(0)), SmallHashMap<int, int, 8>::const_iterator>);
    auto it = cm.find(3);
    REQUIRE(it != cm.end());
    REQUIRE(it->first == 3);
    REQUIRE(it->second == 30);
    REQUIRE((*it).second == 30);

    // a miss compares equal to the const end()
    REQUIRE(cm.find(99999) == cm.end());
}

TEST_CASE("SmallHashMap const find and at after spill")
{
    SmallHashMap<int, int, 2> m; // tiny inline buffer -> will spill to the heap
    for (int i = 0; i < 50; ++i)
        m[i] = i + 1;
    REQUIRE(m.capacity() > 4); // confirm it spilled

    // The const overloads must work the same once the buffer lives on the heap.
    const auto &cm = m;
    for (int i = 0; i < 50; ++i)
    {
        REQUIRE(cm.find(i) != cm.end());
        REQUIRE(cm.find(i)->second == i + 1);
        REQUIRE(cm.at(i) == i + 1);
    }
    REQUIRE(cm.find(50) == cm.end());
}
