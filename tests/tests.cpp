#define CATCH_CONFIG_RUNNER
#include "catch2/catch2.hpp"

#include <sst/cpputils.h>

TEST_CASE("Add One")
{
   REQUIRE( sst::cpputils::addone(1) == 2);
}
int main(int argc, char **argv)
{
    int result = Catch::Session().run(argc, argv);
    return result;
}
