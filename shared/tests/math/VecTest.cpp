#include "shared/math/Vec.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Vec can be aggregate initialized")
{
    s2d::math::Vec<int, 5> vecInt5{1, 2, 3, 4, 5};
    s2d::math::Vec<double, 6> vecDouble6{1.0, 2.2, 3.3, 4.4, 5.9999};
}

TEST_CASE("Vec2 has x and y fields")
{
    s2d::math::Vec2<int> vec{1, 2};
    REQUIRE(vec.x == 1);
    REQUIRE(vec.y == 2);
}

TEST_CASE("Vec3 has x, y and z field")
{
    s2d::math::Vec3<int> vec{1, 2, 3};
    REQUIRE(vec.x == 1);
    REQUIRE(vec.y == 2);
    REQUIRE(vec.z == 3);
}
