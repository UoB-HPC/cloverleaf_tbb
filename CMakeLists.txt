cmake_minimum_required(VERSION 3.17 FATAL_ERROR)
include(FetchContent)

project(cloverleaf_stdpar)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FetchContent_Declare(
        TBB
        GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
        GIT_TAG faaf43c4ab22cb4b4267d65d5e218fa58800eea8
)

# Not using FetchContent_MakeAvailable because we need EXCLUDE_FROM_ALL
FetchContent_GetProperties(TBB)
if (NOT TBB_POPULATED)
    FetchContent_Populate(TBB)
    add_subdirectory(${tbb_SOURCE_DIR} ${tbb_BINARY_DIR} EXCLUDE_FROM_ALL)
endif ()


if (MPI_AS_LIBRARY)

    if (NOT DEFINED MPI_C_LIB_DIR)
        message(FATAL_ERROR "MPI_C_LIB_DIR must be specified, typically <mpi_root_dir>/lib")
    endif ()

    if (NOT DEFINED MPI_C_INCLUDE_DIR)
        message(FATAL_ERROR "MPI_C_INCLUDE_DIR must be specified, typically <mpi_root_dir>/include")
    endif ()

    if (NOT DEFINED MPI_C_LIB)
        message(FATAL_ERROR "MPI_C_LIB must be specified, for example: mpich for libmpich.so in MPI_C_LIB_DIR")
    endif ()

    message(STATUS "Using MPI as a library (${MPI_C_LIB})")
    message(STATUS "MPI include dir: ${MPI_C_INCLUDE_DIR}")
    message(STATUS "MPI library dir: ${MPI_C_LIB_DIR}")

    include_directories(${MPI_C_INCLUDE_DIR})
    link_directories(${MPI_C_LIB_DIR})
else ()
    find_package(MPI REQUIRED)
    set(MPI_C_LIB MPI::MPI_CXX)
endif ()

set(SOURCES
        src/accelerate.cpp
        src/advec_cell.cpp
        src/advec_mom.cpp
        src/advection.cpp
        src/build_field.cpp
        src/calc_dt.cpp
        src/clover_leaf.cpp
        src/comms.cpp
        src/field_summary.cpp
        src/flux_calc.cpp
        src/generate_chunk.cpp
        src/hydro.cpp
        src/ideal_gas.cpp
        src/initialise_chunk.cpp
        src/initialise.cpp
        src/pack_kernel.cpp
        src/PdV.cpp
        src/read_input.cpp
        src/report.cpp
        src/reset_field.cpp
        src/revert.cpp
        src/start.cpp
        src/timer.cpp
        src/timestep.cpp
        src/update_halo.cpp
        src/update_tile_halo.cpp
        src/update_tile_halo_kernel.cpp
        src/viscosity.cpp
        src/visit.cpp)

add_executable(clover_leaf ${SOURCES})

target_include_directories(clover_leaf PUBLIC src)

if (USE_VECTOR)
    target_compile_definitions(clover_leaf PUBLIC USE_VECTOR)
endif ()

target_compile_options(clover_leaf
        PUBLIC
        -Wall
        -Wextra
        -Wcast-align
        #        -Wfatal-errors
        -Werror=return-type
        -Wno-unused-parameter
        -Wno-unused-variable
        #        -Wno-ignored-attributes
        ${EXTRA_FLAGS}
        )

target_include_directories(clover_leaf PUBLIC BEFORE ${TBB_SOURCE_DIR})

set(DEBUG_OPTIONS -O2 ${CXX_EXTRA_FLAGS})
set(RELEASE_OPTIONS -O3 ${CXX_EXTRA_FLAGS})

target_link_libraries(clover_leaf PUBLIC ${MPI_C_LIB} TBB::tbb)
#target_link_libraries(clover_leaf PUBLIC OpenMP::OpenMP_CXX OpenMP::OpenMP_C)

target_compile_options(clover_leaf PUBLIC "$<$<CONFIG:RelWithDebInfo>:${RELEASE_OPTIONS}>")
target_compile_options(clover_leaf PUBLIC "$<$<CONFIG:Release>:${RELEASE_OPTIONS}>")
target_compile_options(clover_leaf PUBLIC "$<$<CONFIG:Debug>:${DEBUG_OPTIONS}>")

target_link_options(clover_leaf PUBLIC LINKER:${CXX_EXTRA_LINKER_FLAGS})

