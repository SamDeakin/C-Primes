cmake_minimum_required(VERSION 3.6)
project(C+Primes)

set(CMAKE_CXX_STANDARD 14)

include(${CMAKE_CURRENT_SOURCE_DIR}/lib/tbb/cmake/TBBBuild.cmake)
tbb_build(TBB_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/lib/tbb CONFIG_DIR TBB_DIR)
find_package(TBB REQUIRED)

set(SOURCE_FILES
        main.cpp
        Worker.cpp
        ThreadPool.cpp)

add_executable(CPPrimes ${SOURCE_FILES})

target_link_libraries(CPPrimes ${TBB_IMPORTED_TARGETS})
