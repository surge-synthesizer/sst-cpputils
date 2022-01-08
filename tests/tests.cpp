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

TEST_CASE("StrSplit")
{
    SECTION("Basic Split in Three")
    {
        int ct{0};
        for (auto q : sst::cpputils::strsplit(std::string("a,a,a"), ','))
        {
            ct++;
            REQUIRE(q == "a");
        }
        REQUIRE(ct == 3);

        auto q = sst::cpputils::strsplit(std::string("a,a,a"), ',');
        for (auto i = q.begin(); i != q.end(); ++i)
        {
            REQUIRE(*i == "a");
        }

        auto v = std::vector<std::string>{"a", "very", "fun", "tool"};
        auto s = std::string("a;very;fun;tool");
        auto z = sst::cpputils::strsplit(s, ';');
        ct = 0;
        for (auto i = z.begin(); i != z.end(); ++i)
        {
            REQUIRE(v[ct] == *i);
            ++ct;
        }

        for (const auto &[i, q] : sst::cpputils::enumerate(z))
        {
            REQUIRE(v[i] == q);
        }
    }

    SECTION("Failed Splits")
    {
        int ct = 0;
        for (auto q : sst::cpputils::strsplit(std::string("This one doesnt split"), '!'))
        {
            REQUIRE(q == "This one doesnt split");
            ct++;
        }
        REQUIRE(ct == 1);

        ct = 0;
        for (auto q : sst::cpputils::strsplit(std::string(""), '!'))
        {
            REQUIRE(q == "");
            ct++;
        }
        REQUIRE(ct == 1);
    }

    SECTION("Split other things")
    {
        std::vector<int> data{0, 1, 17, 2, 3, 17, 4, 5};
        int ct = 0;
        for (const auto &[i, q] : sst::cpputils::enumerate(sst::cpputils::strsplit(data, 17)))
        {
            REQUIRE(q.size() == 2);
            REQUIRE(q[0] == i * 2);
            REQUIRE(q[1] == i * 2 + 1);
            ct++;
        }
        REQUIRE(ct == 3);
    }
}
int main(int argc, char **argv)
{
    int result = Catch::Session().run(argc, argv);
    return result;
}
