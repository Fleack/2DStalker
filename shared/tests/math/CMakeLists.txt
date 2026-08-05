s2d_add_test(shared-math-vec-test
        SOURCES
        VecTest.cpp)

add_custom_target(shared-math-tests)
add_dependencies(shared-math-tests
        shared-math-vec-test
)