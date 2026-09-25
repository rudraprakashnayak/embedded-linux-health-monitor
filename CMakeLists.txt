cmake_minimum_required(VERSION 3.10)
project(device_health_monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(MONITOR_SOURCES
    src/logger.cpp
    src/config_manager.cpp
    src/cpu_monitor.cpp
    src/memory_monitor.cpp
    src/disk_monitor.cpp
    src/temperature_monitor.cpp
    src/network_monitor.cpp
    src/service_monitor.cpp
)

add_executable(device-health-monitor
    src/main.cpp
    ${MONITOR_SOURCES}
)

target_include_directories(device-health-monitor PRIVATE include)

if(UNIX)
    target_link_libraries(device-health-monitor PRIVATE pthread)
endif()

install(TARGETS device-health-monitor RUNTIME DESTINATION bin)
install(FILES config/health_monitor.json DESTINATION /etc/device-health-monitor)
install(FILES systemd/device-health-monitor.service DESTINATION /lib/systemd/system)

enable_testing()

add_executable(test_cpu tests/test_cpu.cpp src/cpu_monitor.cpp)
target_include_directories(test_cpu PRIVATE include)

add_executable(test_memory tests/test_memory.cpp src/memory_monitor.cpp)
target_include_directories(test_memory PRIVATE include)

add_executable(test_config tests/test_config.cpp src/config_manager.cpp)
target_include_directories(test_config PRIVATE include)

add_executable(test_service_monitor tests/test_service_monitor.cpp src/service_monitor.cpp)
target_include_directories(test_service_monitor PRIVATE include)

add_test(NAME test_cpu COMMAND test_cpu)
add_test(NAME test_memory COMMAND test_memory)
add_test(NAME test_config COMMAND test_config)
add_test(NAME test_service_monitor COMMAND test_service_monitor)
