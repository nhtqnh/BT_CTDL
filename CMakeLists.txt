cmake_minimum_required(VERSION 3.16)
project(ClinicQueueSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(BT_CTDL
    main.cpp
    src/models/Patient.cpp
    src/models/Room.cpp
    src/structures/LinkedQueue.cpp
    src/structures/BinaryMinHeap.cpp
    src/algorithms/SortUtils.cpp
    src/services/Hospital.cpp
    src/services/SimulationEngine.cpp
    src/services/StatisticsService.cpp
    src/services/ScenarioRunner.cpp
    src/services/FileManager.cpp
    src/ui/Menu.cpp
)

target_include_directories(BT_CTDL PRIVATE include)

if (MSVC)
    target_compile_options(BT_CTDL PRIVATE /W4)
else()
    target_compile_options(BT_CTDL PRIVATE -Wall -Wextra -Wpedantic)
endif()
