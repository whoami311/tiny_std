#include <catch2/catch_test_macros.hpp>

#include "smart_ptr/unique_ptr.h"

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}

TEST_CASE( "Factorials are computed", "[factorial]" ) {
    tiny_std::unique_ptr<int> ptr;
    REQUIRE( ptr == nullptr );
}
