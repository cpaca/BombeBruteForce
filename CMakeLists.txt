################################################################################
# Lots of this CMakeLists.txt was taken from the Z3Prover CMakeLists tutorial
# Or the Z3 C++ Example Project
# Z3Prover CMakeLists Tutorial: https://github.com/Z3Prover/z3/blob/master/README-CMake.md
# Z3 C++ Example Project: https://github.com/Z3Prover/z3/tree/master/examples/c%2B%2B
################################################################################
cmake_minimum_required(VERSION 3.23)
project(BombeBruteForce)

include(FetchContent)

# Taken from the Z3Prover CMakeLists Tutorial
FetchContent_Declare(z3
        GIT_REPOSITORY https://github.com/Z3Prover/z3
        GIT_TAG        z3-4.12.1
        )
FetchContent_MakeAvailable(z3)

set(CMAKE_CXX_STANDARD 14)

# Taken from the Z3 C++ Example Project
message(STATUS "Z3_FOUND: ${Z3_FOUND}")
message(STATUS "Found Z3 ${Z3_VERSION_STRING}")
message(STATUS "Z3_DIR: ${Z3_DIR}")

# Taken from the Z3Prover CMakeLists Tutorial
include_directories( ${z3_SOURCE_DIR}/src/api )
# I added this next line myself because the above wasn't working
# include_directories( ${z3_SOURCE_DIR}/src/api/c++ )

add_executable(BombeBruteForce main.cpp)
# Taken from the Z3 C++ Example Project
target_include_directories(BombeBruteForce PRIVATE ${Z3_CXX_INCLUDE_DIRS})
target_link_libraries(BombeBruteForce PRIVATE ${Z3_LIBRARIES})
# Taken from the Z3Prover CMakeLists Tutorial
# target_link_libraries(BombeBruteForce libz3)

# Taken from the Z3 C++ Example Project
target_compile_options(BombeBruteForce PRIVATE ${Z3_COMPONENT_CXX_FLAGS})

# Taken from the Z3 C++ Example Project
if (CMAKE_SYSTEM_NAME MATCHES "[Ww]indows")
    # On Windows we need to copy the Z3 libraries
    # into the same directory as the executable
    # so that they can be found.
    foreach (z3_lib ${Z3_LIBRARIES})
        message(STATUS "Adding copy rule for ${z3_lib}")
        add_custom_command(TARGET cpp_example
                POST_BUILD
                COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${z3_lib}>
                $<TARGET_FILE_DIR:cpp_example>
                )
    endforeach()
endif()
