################################################################################
# Lots of this CMakeLists.txt was taken from one of the following files.
# Z3 C++ Example Project: https://github.com/Z3Prover/z3/tree/master/examples/c%2B%2B
################################################################################
cmake_minimum_required(VERSION 3.16)
project(BombeBruteForce)

message("Root Directory: ${CMAKE_CURRENT_BINARY_DIR}")
message("Z3 Directory: ${Z3_DIR}")
message("The above two messages provided because it's possible you, like I, gave it the windows filename (C:\) but it wants the linux filename (mnt/c/)")
# Taken from the Z3 C++ Example Project
find_package(Z3 REQUIRED CONFIG)

# The Z3 C++ Example Project uses C++ 11 like this, so I am too.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Taken from the Z3 C++ Example Project
message(STATUS "Z3_FOUND: ${Z3_FOUND}")
message(STATUS "Found Z3 ${Z3_VERSION_STRING}")
message(STATUS "Z3_DIR: ${Z3_DIR}")

add_executable(BombeBruteForce main.cpp RegionTypes/RegionType.cpp RegionTypes/RegionType.h
        RegionTypes/EqualsRegionType.cpp RegionTypes/EqualsRegionType.h RegionManager.cpp RegionManager.h)
# Taken from the Z3 C++ Example Project
target_include_directories(BombeBruteForce PRIVATE ${Z3_CXX_INCLUDE_DIRS})
target_link_libraries(BombeBruteForce PRIVATE ${Z3_LIBRARIES})

# Taken from the Z3 C++ Example Project
target_compile_options(BombeBruteForce PRIVATE ${Z3_COMPONENT_CXX_FLAGS})

# Taken from the Z3 C++ Example Project
if (CMAKE_SYSTEM_NAME MATCHES "[Ww]indows")
    # On Windows we need to copy the Z3 libraries
    # into the same directory as the executable
    # so that they can be found.
    foreach (z3_lib ${Z3_LIBRARIES})
        message(STATUS "Adding copy rule for ${z3_lib}")
        add_custom_command(TARGET BombeBruteForce
                POST_BUILD
                COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${z3_lib}>
                $<TARGET_FILE_DIR:BombeBruteForce>
                )
    endforeach()
endif()
