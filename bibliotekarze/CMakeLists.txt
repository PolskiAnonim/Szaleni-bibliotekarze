cmake_minimum_required(VERSION 3.19)
project(bibliotekarze)

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{CMAKE_TOOLCHAIN_FILE})
    set(CMAKE_TOOLCHAIN_FILE $ENV{CMAKE_TOOLCHAIN_FILE})
endif ()

set(CMAKE_C_COMPILER /usr/lib64/mpi/gcc/openmpi4/bin/mpicc)
set(CMAKE_CXX_COMPILER /usr/lib64/mpi/gcc/openmpi4/bin/mpic++)

if (UNIX)
    set(CMAKE_CXX_STANDARD 17)

    find_package(MPI REQUIRED)
    file(GLOB SRC CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.h"
    )
    set(CMAKE_CXX_FLAGS -pthread)

    add_executable(bibliotekarze "${SRC}")
    target_include_directories(bibliotekarze PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/src/communication
            ${CMAKE_SOURCE_DIR}/src/domain
    )

    target_link_libraries(bibliotekarze PUBLIC MPI::MPI_CXX)

    find_program(iwyu_path NAMES include-what-you-use iwyu)
    if (iwyu_path)
        set_property(TARGET bibliotekarze PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
    endif ()
endif ()
