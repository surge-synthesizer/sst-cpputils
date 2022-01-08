#define CATCH_CONFIG_RUNNER
#include "catch2/catch2.hpp"

#include <sst/cpputils.h>
#include <algorithm>
#include <array>

TEST_CASE("Enumerate")
{
   SECTION( "Simple Vector" )
   {
       std::vector<int> v{0,1,2,3};
       for (const auto [idx, val] : sst::cpputils::enumerate(v))
       {
           REQUIRE( idx == val);
       }

       std::vector<int> vOut;
       std::transform(v.begin(), v.end(), std::back_inserter(vOut), [](auto a) { return a * 2; });
       for (const auto [idx, val] : sst::cpputils::enumerate(vOut))
       {
           REQUIRE( idx * 2 == val);
       }
   }

   SECTION( "Some other types" )
   {
       std::string abcs = "abcdefg";
       for (const auto [idx, ch] : sst::cpputils::enumerate(abcs))
       {
           REQUIRE( ch == idx + 'a');
       }

       std::array<char, 4> abcarr{'d','e','f','g'};
       for (const auto [idx, ch] : sst::cpputils::enumerate(abcarr))
       {
           REQUIRE( ch == idx + 'd');
       }
   }

   SECTION( "Empty Containers" )
   {
       auto empty = [](const auto &v)
       {
           bool never{true};
           for (const auto [i,j] : sst::cpputils::enumerate(v))
           {
               never = false;
           }
           REQUIRE( never);
       };
       empty(std::vector<int>());
       empty(std::string());
       empty(std::array<int,0>());
   }
}
int main(int argc, char **argv)
{
    int result = Catch::Session().run(argc, argv);
    return result;
}
