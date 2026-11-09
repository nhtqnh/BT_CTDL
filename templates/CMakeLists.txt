cmake_minimum_required(VERSION 3.16)
project(ClinicQueueSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Bo sung cac tep .cpp khi nhom da tao module tuong ung.
add_executable(clinic_queue
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

target_include_directories(clinic_queue PRIVATE include)

# Khuyen nghi bat canh bao trong qua trinh phat trien.
if (MSVC)
    target_compile_options(clinic_queue PRIVATE /W4)
else()
    target_compile_options(clinic_queue PRIVATE -Wall -Wextra -Wpedantic)
endif()
