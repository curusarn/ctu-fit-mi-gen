cmake_minimum_required(VERSION 2.8.8)

add_library(MyPasses MODULE
    # List your source files here.
    src/MyPasses.cc
)

# set C++11 definitions, error on everything, enable debugging
add_definitions(--std=c++11 -pedantic -Wall -Werror -g)

# Use C++11 to compile your pass (i.e., supply -std=c++11).
target_compile_features(MyPasses PRIVATE cxx_range_for cxx_auto_type)

