cmake_minimum_required(VERSION 3.20)
project(heat_diffusion LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(MPI REQUIRED COMPONENTS CXX)

function(enable_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
                -Wall -Wextra -Wpedantic
        )
    endif()
endfunction()

add_library(heatPlate
        src/heatPlate.cpp
)

target_include_directories(heatPlate
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

enable_warnings(heatPlate)

add_executable(heat_sim
        src/main.cpp
)

target_link_libraries(heat_sim PRIVATE heatPlate)

enable_warnings(heat_sim)

add_library(mpiHeatPlate
        src/mpiHeatPlate.cpp
)

target_include_directories(mpiHeatPlate
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(mpiHeatPlate
        PUBLIC MPI::MPI_CXX
)

enable_warnings(mpiHeatPlate)

add_executable(heat_sim_mpi
        src/main_mpi.cpp
)

target_link_libraries(heat_sim_mpi
        PRIVATE mpiHeatPlate
)

enable_warnings(heat_sim_mpi)

include(CTest)

if(BUILD_TESTING)
    add_executable(test_mpiHeatPlate
            tests/test_mpiHeatPlate.cpp
    )
    add_test(
            NAME heat_plate_tests
            COMMAND test_heatPlate
    )

    target_link_libraries(test_mpiHeatPlate
            PRIVATE mpiHeatPlate heatPlate
    )

    enable_warnings(test_mpiHeatPlate)

    foreach(np IN ITEMS 1 2 4)
        add_test(
                NAME mpi_heat_plate_np${np}
                COMMAND
                ${MPIEXEC_EXECUTABLE}
                ${MPIEXEC_NUMPROC_FLAG} ${np}
                ${MPIEXEC_PREFLAGS}
                $<TARGET_FILE:test_mpiHeatPlate>
                ${MPIEXEC_POSTFLAGS}
        )

        set_tests_properties(mpi_heat_plate_np${np}
                PROPERTIES
                PROCESSORS ${np}
                LABELS "mpi"
        )
    endforeach()
endif()