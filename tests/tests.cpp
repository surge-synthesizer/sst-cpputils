#define CATCH_CONFIG_RUNNER
#include "catch2/catch2.hpp"

#include <sst/cpputils.h>

#include <algorithm>
#include <array>
#include <map>
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
}

TEST_CASE("Erase")
{
    SECTION("Simple Vector")
    {
        std::vector<int> v{1, 3, 5};
        sst::cpputils::nodal_erase_if(v, [] (int x) { return x > 2 && x < 4; });

        REQUIRE(v[0] == 1);
        REQUIRE(v[1] == 5);
        REQUIRE(v.size() == 2);
    }

    SECTION("Some other types")
    {
        std::string abcs = "abcdefg";
        sst::cpputils::nodal_erase_if(abcs, [] (char ch) { return ch > 'a' && ch < 'g'; });

        REQUIRE(abcs[0] == 'a');
        REQUIRE(abcs[1] == 'g');
        REQUIRE(abcs.size() == 2);
    }

    SECTION("Map Erase")
    {
        // std::remove_if doesn't like types with user-defined move constructor, like this one:
        struct TestStruct
        {
            std::string x;

            TestStruct() = default;
            TestStruct (const TestStruct&) = delete;
            TestStruct& operator= (const TestStruct&) = delete;
            TestStruct (TestStruct&&) noexcept = default;
            TestStruct& operator= (TestStruct&&) noexcept = default;
        };

        std::map<int, TestStruct> m;
        m[1] = TestStruct { "there" };
        m[2] = TestStruct { "keeper" };

        // we want to do this, but compiler will give errors.
        // m.erase(std::remove_if(m.begin(), m.end(), [] (const auto& pair) { return pair.second.x == "keeper"; }), m.end());

        sst::cpputils::nodal_erase_if(m, [] (const auto& pair) { return pair.second.x == "keeper"; });

        REQUIRE(m[1].x == "there");
        REQUIRE(m.size() == 1);
    }
}

int main(int argc, char **argv)
{
    int result = Catch::Session().run(argc, argv);
    return result;
}
