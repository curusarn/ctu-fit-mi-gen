cmake_minimum_required(VERSION 2.8.8)

add_library(MyPasses MODULE
    # List your source files here.
    src/MyPasses.cc
)

# Use C++11 to compile your pass (i.e., supply -std=c++11).
target_compile_features(MyPasses PRIVATE cxx_range_for cxx_auto_type)

