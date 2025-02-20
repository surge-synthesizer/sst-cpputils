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
#define CATCH_CONFIG_RUNNER
#include "catch2.hpp"

#include <sst/cpputils.h>

#include <algorithm>
#include <array>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string>

TEST_CASE("Enumerate")
{
    SECTION("Simple Vector")
    {
        std::vector<int> v{0, 1, 2, 3};
        for (const auto [idx, val] : sst::cpputils::enumerate(v))
        {
            REQUIRE(idx == val);
        }

        std::vector<int> vOut;
        std::transform(v.begin(), v.end(), std::back_inserter(vOut), [](auto a) { return a * 2; });
        for (const auto [idx, val] : sst::cpputils::enumerate(vOut))
        {
            REQUIRE(idx * 2 == val);
        }
    }

    SECTION("Some other types")
    {
        std::string abcs = "abcdefg";
        for (const auto [idx, ch] : sst::cpputils::enumerate(abcs))
        {
            REQUIRE(ch == idx + 'a');
        }

        std::array<char, 4> abcarr{'d', 'e', 'f', 'g'};
        for (const auto [idx, ch] : sst::cpputils::enumerate(abcarr))
        {
            REQUIRE(ch == idx + 'd');
        }
    }

    SECTION("Empty Containers")
    {
        auto empty = [](const auto &v) {
            bool never{true};
            for (const auto [i, j] : sst::cpputils::enumerate(v))
            {
                never = false;
            }
            REQUIRE(never);
        };
        empty(std::vector<int>());
        empty(std::string());
        empty(std::array<int, 0>());
    }

    SECTION("Enumerate Map")
    {
        std::map<std::string, std::string> m;
        m["hi"] = "there";
        m["zoo"] = "keeper";

        for (const auto [a, p] : sst::cpputils::enumerate(m))
        {
            auto [k, v] = p;
            if (a == 0)
            {
                REQUIRE(k == "hi");
                REQUIRE(v == "there");
            }
            if (a == 1)
            {
                REQUIRE(k == "zoo");
                REQUIRE(v == "keeper");
            }
        }
    }
}

TEST_CASE("Zip")
{
    SECTION("ZIP with Self")
    {
        auto selfzip = [](const auto &v) {
            int ct = 0;
            for (const auto &[a, b] : sst::cpputils::zip(v, v))
            {
                REQUIRE(a == b);
                ct++;
            }
            REQUIRE(ct == v.size());
        };
        selfzip(std::vector<int>{1, 2, 3});
        selfzip(std::vector<int>());
        selfzip(std::string("hello world"));
        selfzip(std::array<int, 3>{3, 2, 4});
    }
    SECTION("Simple Pair of Vectors")
    {
        std::vector<int> v0{0, 1, 2}, v1{0, 2, 4};
        for (const auto &[a, b] : sst::cpputils::zip(v0, v1))
        {
            REQUIRE(a * 2 == b);
        }
    }

    SECTION("Varying Types")
    {
        std::vector<int> v0{0, 2, 4, 6, 8};
        std::string msg = "acegi";
        for (const auto &[a, b] : sst::cpputils::zip(msg, v0))
        {
            REQUIRE(a == b + 'a');
        }
        for (const auto &[a, b] : sst::cpputils::zip(v0, msg))
        {
            REQUIRE(a + 'a' == b);
        }
    }

    SECTION("Varying Lengths")
    {
        std::vector<int> v0{0, 2, 4}, v1{0, 1, 2, 3, 4, 5};

        int ct{0};
        for (const auto &[a, b] : sst::cpputils::zip(v0, v1))
        {
            REQUIRE(a == b * 2);
            ct++;
        }
        REQUIRE(ct == std::min(v0.size(), v1.size()));

        ct = 0;
        for (const auto &[a, b] : sst::cpputils::zip(v1, v0))
        {
            REQUIRE(a * 2 == b);
            ct++;
        }
        REQUIRE(ct == std::min(v0.size(), v1.size()));

        for (const auto &[a, b] : sst::cpputils::zip(v0, "ace ventura"))
        {
            REQUIRE(a + 'a' == b);
        }
    }

    SECTION("ZIP with Empty")
    {
        std::vector<int> test{0, 1, 2}, empty;
        for (const auto &[a, b] : sst::cpputils::zip(test, empty))
        {
            REQUIRE(false);
        }
        for (const auto &[a, b] : sst::cpputils::zip(empty, test))
        {
            REQUIRE(false);
        }
    }
}

TEST_CASE("Contains")
{
    SECTION("Simple Vector")
    {
        std::vector<int> v{1, 3, 5, 7};

        REQUIRE(sst::cpputils::contains(v, 3));
        REQUIRE(!sst::cpputils::contains(v, 2));

        auto isEven = [](const auto &x) { return x % 2 == 0; };
        auto isOdd = [](const auto &x) { return x % 2 == 1; };
        REQUIRE(sst::cpputils::contains_if(v, isOdd));
        REQUIRE(!sst::cpputils::contains_if(v, isEven));
    }

    SECTION("Some other types")
    {
        std::string abcs = "abcdefg";
        REQUIRE(sst::cpputils::contains(abcs, 'e'));
        REQUIRE(!sst::cpputils::contains(abcs, 'y'));

        std::array<char, 4> abcarr{'d', 'e', 'f', 'g'};
        REQUIRE(sst::cpputils::contains(abcarr, 'e'));
        REQUIRE(!sst::cpputils::contains(abcarr, 'y'));
    }

    SECTION("Empty Containers")
    {
        auto empty = [](const auto &v, auto dummyVal) {
            REQUIRE(!sst::cpputils::contains(v, dummyVal));
        };
        empty(std::vector<int>(), 0);
        empty(std::string(), char());
        empty(std::array<int, 0>(), 0);
    }

    SECTION("Map Contains")
    {
        std::map<std::string, std::string> m;
        m["hi"] = "there";
        m["zoo"] = "keeper";

        REQUIRE(sst::cpputils::contains_if(m, [](const auto &pair) { return pair.first == "hi"; }));
        REQUIRE(!sst::cpputils::contains_if(
            m, [](const auto &pair) { return pair.first == "not_a_key"; }));

        REQUIRE(sst::cpputils::contains_if(
            m, [](const auto &pair) { return pair.second == "keeper"; }));
        REQUIRE(!sst::cpputils::contains_if(
            m, [](const auto &pair) { return pair.second == "not_a_value"; }));
    }
}

TEST_CASE("SimpleRingBuffer")
{
    SECTION("Pop of empty buffer has no value")
    {
        sst::cpputils::SimpleRingBuffer<float, 8> buf;
        REQUIRE(!buf.pop().has_value());
    }

    SECTION("Push and pop work as expected")
    {
        // Basics.
        sst::cpputils::SimpleRingBuffer<int, 4> buf;
        buf.push(0);
        buf.push(1);
        REQUIRE(*buf.pop() == 0);
        REQUIRE(*buf.pop() == 1);

        // Get the write marker past the end.
        buf.push(2);
        buf.push(3);
        buf.push(4);
        REQUIRE(*buf.pop() == 2);
        REQUIRE(*buf.pop() == 3);
        REQUIRE(*buf.pop() == 4);

        // Should be empty.
        REQUIRE(!buf.pop().has_value());

        // Write one past the full capacity. Should be marked as empty.
        buf.push(5);
        buf.push(6);
        buf.push(7);
        buf.push(8);
        REQUIRE(!buf.pop().has_value());

        // Now write another one, the read value should be skipped to the end and we're empty,
        // skipping the entire previous batch of writes.
        buf.push(9);
        REQUIRE(*buf.pop() == 9);
        REQUIRE(!buf.pop().has_value());
    }

    SECTION("Popall works as expected")
    {
        sst::cpputils::SimpleRingBuffer<int, 4> buf;
        buf.push(0);
        buf.push(1);
        buf.push(2);
        REQUIRE_THAT(buf.popall(), Catch::Matchers::Equals(std::vector<int>{0, 1, 2}));
        REQUIRE(buf.popall().empty());
        REQUIRE(!buf.pop().has_value());

        // Let's go fully around the buffer and see what we get.
        buf.push(3);
        buf.push(4);
        buf.push(5);
        buf.push(6);
        REQUIRE(buf.empty());
        REQUIRE(buf.popall().empty());
        buf.push(7);
        buf.push(8);
        REQUIRE_THAT(buf.popall(), Catch::Matchers::Equals(std::vector<int>{7, 8}));
    }

    SECTION("Pushall works as expected")
    {
        sst::cpputils::SimpleRingBuffer<int, 4> buf;
        buf.push(0);
        std::vector<int> v = {1, 2, 3, 4};
        buf.push(v);
        // Should have wrapped around once.
        REQUIRE_THAT(buf.popall(), Catch::Matchers::Equals(std::vector<int>{4}));

        buf.push(v);
        // Should have come back around the ring. Read pointer wouldn't have moved, so we'd be
        // considered empty.
        REQUIRE(buf.empty());

        // Smaller buffer works as expect.
        buf.clear();
        v = {1, 2, 3};
        buf.push(v);
        REQUIRE_THAT(buf.popall(), Catch::Matchers::Equals(v));

        // Try a super long push.
        buf.clear();
        v = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        buf.push(v);
        // size = 11, so should go around twice before adding anything.
        REQUIRE_THAT(buf.popall(), Catch::Matchers::Equals(std::vector<int>{8, 9, 10}));

        // Should get the last 2 vals when 2 already in, and go around 3x.
        buf.clear();
        buf.push(0);
        buf.push(1);
        v = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        buf.push(v);
        REQUIRE_THAT(buf.popall(), Catch::Matchers::Equals(std::vector<int>{10, 11}));
    }

    SECTION("Non-copyable")
    {
        sst::cpputils::SimpleRingBuffer<std::unique_ptr<int>, 4> buf;
        auto z = std::make_unique<int>(5);
        buf.push(std::move(z));
    }
}

// Repeat of the RingBuffer tests with the StereoRingBuffer.
TEST_CASE("StereoRingBuffer")
{
    SECTION("Pop of empty buffer has no value")
    {
        sst::cpputils::StereoRingBuffer<float, 8> buf;
        REQUIRE(!buf.pop().has_value());
    }

    SECTION("Push and pop work as expected")
    {
        using P = std::pair<float, float>;

        // Basics.
        sst::cpputils::StereoRingBuffer<float, 4> buf;
        buf.push(0, 1);
        buf.push(2, 3);
        REQUIRE(*buf.pop() == P{0, 1});
        REQUIRE(*buf.pop() == P{2, 3});

        // Get the write marker past the end.
        buf.push(2, 2);
        buf.push(3, 3);
        buf.push(4, 4);
        REQUIRE(*buf.pop() == P{2, 2});
        REQUIRE(*buf.pop() == P{3, 3});
        REQUIRE(*buf.pop() == P{4, 4});

        // Should be empty.
        REQUIRE(!buf.pop().has_value());

        // Write one past the full capacity. Should be marked as empty.
        buf.push(5, 5);
        buf.push(6, 6);
        buf.push(7, 7);
        buf.push(8, 8);
        REQUIRE(!buf.pop().has_value());

        // Now write another one, the read value should be skipped to the end and we're empty,
        // skipping the entire previous batch of writes.
        buf.push(9, 9);
        REQUIRE(*buf.pop() == P{9, 9});
        REQUIRE(!buf.pop().has_value());
    }

    SECTION("Popall works as expected")
    {
        sst::cpputils::StereoRingBuffer<float, 4> buf;
        buf.push(0, 1);
        buf.push(2, 3);
        buf.push(4, 5);
        auto expected = std::make_pair(std::vector<float>{0, 2, 4}, std::vector<float>{1, 3, 5});
        auto result = buf.popall();
        REQUIRE_THAT(result.first, Catch::Matchers::Equals(expected.first));
        REQUIRE_THAT(result.second, Catch::Matchers::Equals(expected.second));
        REQUIRE(buf.popall().first.empty());
        REQUIRE(!buf.pop().has_value());

        // Let's go fully around the buffer and see what we get.
        buf.push(3, 3);
        buf.push(4, 4);
        buf.push(5, 5);
        buf.push(6, 6);
        REQUIRE(buf.empty());
        REQUIRE(buf.popall().first.empty());
        buf.push(7, 7);
        buf.push(8, 8);
        result = buf.popall();
        REQUIRE_THAT(result.first, Catch::Matchers::Equals(std::vector<float>{7, 8}));
        REQUIRE_THAT(result.second, Catch::Matchers::Equals(std::vector<float>{7, 8}));
    }

    SECTION("Pushall works as expected")
    {
        sst::cpputils::StereoRingBuffer<float, 4> buf;
        buf.push(0, 0);
        std::vector<float> vL = {1, 2, 3, 4};
        std::vector<float> vR = {5, 6, 7, 8};
        buf.push(vL, vR);
        // Should have wrapped around once.
        REQUIRE_THAT(buf.popall().first, Catch::Matchers::Equals(std::vector<float>{4}));

        buf.push(vL, vR);
        // Should have come back around the ring. Read pointer wouldn't have moved, so we'd
        // be considered empty.
        REQUIRE(buf.empty());

        // Smaller buffer works as expect.
        buf.clear();
        vL = {1, 2, 3};
        vR = {4, 5, 6};
        buf.push(vL, vR);
        REQUIRE_THAT(buf.popall().second, Catch::Matchers::Equals(vR));

        // Try a super long push.
        buf.clear();
        vL = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        vR = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        buf.push(vL, vR);
        // size = 11, so should go around twice before adding anything.
        REQUIRE_THAT(buf.popall().first, Catch::Matchers::Equals(std::vector<float>{8, 9, 10}));

        // Should get the last 2 vals when 2 already in, and go around 3x.
        buf.clear();
        buf.push(0, 0);
        buf.push(1, 1);
        vL = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        vR = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        buf.push(vL, vR);
        REQUIRE_THAT(buf.popall().first, Catch::Matchers::Equals(std::vector<float>{10, 11}));
    }
}

TEST_CASE("Erase")
{
    SECTION("Simple Vector")
    {
        std::vector<int> v{1, 3, 5};
        sst::cpputils::nodal_erase_if(v, [](int x) { return x > 2 && x < 4; });

        REQUIRE(v[0] == 1);
        REQUIRE(v[1] == 5);
        REQUIRE(v.size() == 2);
    }

    SECTION("Some other types")
    {
        std::string abcs = "abcdefg";
        sst::cpputils::nodal_erase_if(abcs, [](char ch) { return ch > 'a' && ch < 'g'; });

        REQUIRE(abcs[0] == 'a');
        REQUIRE(abcs[1] == 'g');
        REQUIRE(abcs.size() == 2);
    }

    SECTION("Map Erase")
    {
        // std::remove_if doesn't like types with user-defined move constructor, like this
        // one:
        struct TestStruct
        {
            std::string x;

            TestStruct() = default;

            TestStruct(const TestStruct &) = delete;

            TestStruct &operator=(const TestStruct &) = delete;

            TestStruct(TestStruct &&) noexcept = default;

            TestStruct &operator=(TestStruct &&) noexcept = default;
        };

        std::map<int, TestStruct> m;
        m[1] = TestStruct{"there"};
        m[2] = TestStruct{"keeper"};

        // we want to do this, but compiler will give errors.
        // m.erase(std::remove_if(m.begin(), m.end(), [] (const auto& pair) { return
        // pair.second.x
        // == "keeper"; }), m.end());

        sst::cpputils::nodal_erase_if(m,
                                      [](const auto &pair) { return pair.second.x == "keeper"; });

        REQUIRE(m[1].x == "there");
        REQUIRE(m.size() == 1);
    }
}

TEST_CASE("Bindings")
{
    struct Point
    {
        int x = 0, y = 0;

        void displace(int x_off, int y_off)
        {
            x += x_off;
            y += y_off;
        }
    };

    SECTION("Bind Front Test")
    {
        const auto sum_func = [](int a, int b, int c, int d) { return a + b + c + d; };
        const auto sum_two = sst::cpputils::bind_front(sum_func, 1, 2);
        REQUIRE(sum_two(3, 4) == 10);
    }

    SECTION("Bind Front Struct Test")
    {
        Point p{};
        auto displace = sst::cpputils::bind_front(&Point::displace, &p);

        displace(3, 4);

        REQUIRE(p.x == 3);
        REQUIRE(p.y == 4);
    }

    SECTION("Bind Back Test")
    {
        const auto func = [](int a, int b, int c, int d) { return a + b - c - d; };
        const auto sum_two = sst::cpputils::bind_back(func, 1, 2);
        REQUIRE(sum_two(3, 4) == 4);
    }

    SECTION("Bind Back Struct Test")
    {
        auto displace_3_4 = sst::cpputils::bind_back(&Point::displace, 3, 4);

        Point p{};
        displace_3_4(&p);

        REQUIRE(p.x == 3);
        REQUIRE(p.y == 4);
    }
}

TEST_CASE("LRU")
{
    SECTION("Key-constructed struct")
    {
        struct ts
        {
            explicit ts(const int &k) : key(k) {}

            int key;
        };

        sst::cpputils::LRU<int, ts> cache(3);
        auto s1 = cache.get(1);
        auto s2 = cache.get(2);
        auto s3 = cache.get(3);
        REQUIRE(s1->key == 1);
        REQUIRE(s2->key == 2);
        REQUIRE(s3->key == 3);
        // Move s1 up to head.
        cache.get(1);
        // Should be two use counts for s1, s2, and s3, as they all should be in the cache.
        REQUIRE(s1.use_count() == 2);
        REQUIRE(s2.use_count() == 2);
        REQUIRE(s3.use_count() == 2);
        // Now get a new one, s4. The least recently used should be s2, so it should get evicted and
        // the shared_ptr's use_count should drop to 1. The others should remain two.
        cache.get(4);
        REQUIRE(s1.use_count() == 2);
        REQUIRE(s2.use_count() == 1);
        REQUIRE(s3.use_count() == 2);

        // The following will fail to compile.
        // cache.get(1, 1, 2);
    }

    SECTION("Multiple-constructed struct")
    {
        struct ts3
        {
            ts3(int a, float b, const int &c) : a_(a), b_(b), c_(c) {}

            int a_;
            float b_;
            int c_;
        };

        sst::cpputils::LRU<int, ts3> cache(1);

        // Just make sure the compilation works.
        auto v = cache.get(1, 1, 2.f, 3);
        REQUIRE(v->a_ == 1);
        REQUIRE(v->c_ == 3);

        // The following will fail to compile.
        // cache.get(1);
    }
}

TEST_CASE("Array CTor")
{
    struct NeedsArgs
    {
        int a, b;

        NeedsArgs(int aa, int bb) : a(aa), b(bb) {}

        int val() const { return a * 1000 + b; }
    };

    std::array<NeedsArgs, 20> arr{sst::cpputils::make_array<NeedsArgs, 20>(17, 42)};
    for (const auto &a : arr)
    {
        REQUIRE(a.val() == 17042);
    }
}

TEST_CASE("Array Indexed CTor")
{
    struct NeedsArgs
    {
        int a, b;

        NeedsArgs(int aa, int bb) : a(aa), b(bb) {}

        int val() const { return a * 1000 + b; }
    };

    SECTION("Last Index")
    {
        std::array<NeedsArgs, 20> arr{sst::cpputils::make_array_bind_last_index<NeedsArgs, 20>(17)};
        for (const auto &[idx, a] : sst::cpputils::enumerate(arr))
        {
            REQUIRE(a.val() == 17000 + idx);
        }
    }

    SECTION("First Index")
    {
        std::array<NeedsArgs, 20> arr{
            sst::cpputils::make_array_bind_first_index<NeedsArgs, 20>(23)};
        for (const auto &[idx, a] : sst::cpputils::enumerate(arr))
        {
            REQUIRE(a.val() == 1000 * idx + 23);
        }
    }
}

TEST_CASE("Array Lambda CTor")
{
    struct NeedsArgs
    {
        int a, b;

        NeedsArgs(int aa, int bb) : a(aa), b(bb) {}

        int val() const { return a + b; }
    };

    std::array<NeedsArgs, 20> arr{sst::cpputils::make_array_lambda<NeedsArgs, 20>([](auto idx) {
        return NeedsArgs{(int)idx, (int)idx * 2};
    })};
    for (const auto [idx, a] : sst::cpputils::enumerate(arr))
    {
        REQUIRE(a.val() == 3 * idx);
    }
}

TEST_CASE("Fixed Allocator")
{
    SECTION("Vector")
    {
        using alloc_t = sst::cpputils::fixed_memory_allocator<int, 128>;
        using vec_t = std::vector<int, alloc_t>;

        alloc_t alloc(true);

        vec_t v(alloc);
        auto q = v;

        v.push_back(1);
        q.push_back(2);

        v.push_back(4);
        v.push_back(5);
        REQUIRE(v.size() == 3);
        REQUIRE(q.size() == 1);
        REQUIRE(v[0] == 1);
        REQUIRE(v[1] == 4);
        REQUIRE(q[0] == 2);
    }

    SECTION("List")
    {
        using alloc_t = sst::cpputils::fixed_memory_allocator<int, 4096>;
        using list_t = std::list<int, alloc_t>;

        alloc_t alloc(true);
        list_t v(alloc);
        for (int i = 0; i < 3; i++)
            v.push_back(i);

        v.erase(v.begin());

        v.push_back(10);

        auto it = v.begin();
        it++;
        v.erase(it);
        v.push_back(33);

        REQUIRE(v.size() == 3);
    }

    SECTION("Unordered Map")
    {
        using alloc_t = sst::cpputils::fixed_memory_allocator<std::pair<const int, int>, 4096>;
        using umap_t = std::unordered_map<int, int, std::hash<int>, std::equal_to<int>, alloc_t>;

        alloc_t alloc(true);
        umap_t m(0, alloc);

        for (int i = 0; i < 3; i++)
            m.insert({i, i * 2});

        REQUIRE(m.size() == 3);
        for (int i = 0; i < 3; ++i)
            REQUIRE(m.at(i) == i * 2);
    }
}

TEST_CASE("ActiveSet")
{
    struct TestThing : sst::cpputils::active_set_overlay<TestThing>::participant
    {
        int value;
    };

    std::array<TestThing, 64> things;
    for (int i = 0; i < 64; ++i)
        things[i].value = i;

    auto len = [](const auto &as) {
        int idx = 0;
        for (const auto &x : as)
            idx++;
        return idx;
    };
    SECTION("Two Inserts")
    {
        sst::cpputils::active_set_overlay<TestThing> as;

        REQUIRE(len(as) == 0);
        REQUIRE(len(as) == as.activeCount);
        as.addToActive(things[0]);
        REQUIRE(len(as) == 1);
        REQUIRE(len(as) == as.activeCount);

        as.addToActive(things[7]);
        REQUIRE(len(as) == 2);
        REQUIRE(len(as) == as.activeCount);

        std::unordered_set<int> vals;
        for (auto &x : as)
            vals.insert(x.value);
        REQUIRE(vals.size() == 2);
        REQUIRE(vals.find(0) != vals.end());
        REQUIRE(vals.find(7) != vals.end());
    }

    SECTION("Insert Same Twice")
    {
        sst::cpputils::active_set_overlay<TestThing> as;

        as.addToActive(things[17]);
        REQUIRE(len(as) == 1);
        REQUIRE(len(as) == as.activeCount);

        as.addToActive(things[17]);
        REQUIRE(len(as) == 1);
        REQUIRE(len(as) == as.activeCount);
        REQUIRE(as.begin()->value == 17);
        REQUIRE(&*as.begin() == &things[17]);

        as.removeFromActive(things[17]);
        REQUIRE(len(as) == 0);
        REQUIRE(len(as) == as.activeCount);
    }

    SECTION("Remove Front works")
    {
        sst::cpputils::active_set_overlay<TestThing> as;

        as.addToActive(things[17]);
        REQUIRE(len(as) == 1);
        REQUIRE(len(as) == as.activeCount);

        as.addToActive(things[17]);
        REQUIRE(len(as) == 1);
        REQUIRE(len(as) == as.activeCount);
        REQUIRE(as.begin()->value == 17);
        REQUIRE(&*as.begin() == &things[17]);

        as.addToActive(things[22]);
        REQUIRE(len(as) == 2);
        REQUIRE(len(as) == as.activeCount);
        as.removeFromActive(*as.begin());
        REQUIRE(len(as) == 1);
        REQUIRE(len(as) == as.activeCount);

        as.removeFromActive(things[17]);
        REQUIRE(len(as) == 0);
        REQUIRE(len(as) == as.activeCount);
    }

    SECTION("Drain Front First")
    {
        sst::cpputils::active_set_overlay<TestThing> as;
        for (int i = 0; i < 40; ++i)
        {
            as.addToActive(things[rand() & things.size()]);
        }
        REQUIRE(len(as) <= 40);

        REQUIRE(as.activeCount <= 40);
        while (as.begin() != as.end())
        {
            as.removeFromActive(*as.begin());
        }
        REQUIRE(len(as) == 0);
        REQUIRE(len(as) == as.activeCount);
    }

    SECTION("Insert, then Insert another")
    {
        sst::cpputils::active_set_overlay<TestThing> as;
        as.addToActive(things[17]);
        REQUIRE(len(as) == 1);
        REQUIRE(len(as) == as.activeCount);
        REQUIRE(as.begin()->value == 17);
        as.removeFromActive(things[17]);
        REQUIRE(len(as) == 0);
        REQUIRE(len(as) == as.activeCount);

        as.addToActive(things[21]);
        REQUIRE(len(as) == 1);
        REQUIRE(len(as) == as.activeCount);
        REQUIRE(as.begin()->value == 21);
        REQUIRE(as.removeFromActive(things[21]));
        REQUIRE(len(as) == as.activeCount);
        REQUIRE(len(as) == 0);
    }

    SECTION("Remove without add")
    {
        sst::cpputils::active_set_overlay<TestThing> as;
        REQUIRE(len(as) == 0);
        REQUIRE(len(as) == as.activeCount);
        REQUIRE(!as.removeFromActive(things[17]));
        REQUIRE(len(as) == 0);
        REQUIRE(len(as) == as.activeCount);
    }
}

int main(int argc, char **argv)
{
    int result = Catch::Session().run(argc, argv);
    return result;
}
