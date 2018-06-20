cmake_minimum_required(VERSION 2.8.8)

project(mila+)

message(STATUS "Local configuration:")
# for some reason LLVM cmake script do not include this on mint 17.2
include_directories("../llvm/llvm-src-370/include")
include_directories("${CMAKE_SOURCE_DIR}/src")

# enable this for release LLVM
# message(STATUS "LLVM will be in RELEASE mode")
# set(LLVM_DIR "../llvm/llvm-build-370-nodebug/cmake/modules/CMakeFiles")
# enable this for debug LLVM
message(STATUS "LLVM will be in DEBUG mode")
set(LLVM_DIR "../llvm/llvm-build-370/cmake/modules/CMakeFiles")

# enable this to set compiler to CLANG
message(STATUS "CLANG compiler will be used")
#set(CMAKE_C_COMPILER "clang-3.7")
#set(CMAKE_CXX_COMPILER "clang++-3.7")

# set C++11 definitions, error on everything, enable debugging
add_definitions(--std=c++11 -pedantic -Wall -Werror -g)

# find llvm package and set it up
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# add sources
file(GLOB_RECURSE SRC "src/*.cpp" "src/*.cc" "src/*.h" "tests/*.mila")
add_executable(${PROJECT_NAME} ${SRC})

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(LLVM_LIBS support core mcjit native irreader linker ipo bitwriter)
target_link_libraries(${PROJECT_NAME} ${LLVM_LIBS})
