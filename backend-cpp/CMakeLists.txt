cmake_minimum_required(VERSION 3.16)
project(ProyectoIzana VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(BUILD_TESTS "Build test programs" ON)
option(BUILD_EXAMPLES "Build example programs" ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find WebSocket++ and its dependencies
find_path(WEBSOCKETPP_INCLUDE_DIR websocketpp/config.hpp)
if(NOT WEBSOCKETPP_INCLUDE_DIR)
    message(FATAL_ERROR "WebSocket++ not found. Please install websocketpp-dev package or set WEBSOCKETPP_INCLUDE_DIR manually")
endif()

# Find serial library (libserial or similar)
pkg_check_modules(SERIAL REQUIRED libserial)

# Find JSON library (nlohmann/json)
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    # Fallback to pkg-config
    pkg_check_modules(JSON REQUIRED nlohmann_json)
endif()

# Find threads
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${WEBSOCKETPP_INCLUDE_DIR})

# Source files organization
set(CORE_SOURCES
    src/core/UARTModule.cpp
    src/core/DataProcessor.cpp
    src/core/ParameterManager.cpp
    src/core/WebSocketServer.cpp
    src/core/CircularBuffer.cpp
)

set(PROCESSING_SOURCES
    src/processing/ProcessingModule.cpp
    src/processing/FFTModule.cpp
    src/processing/FilterModule.cpp
    src/processing/StorageModule.cpp
    src/processing/ProcessingPipeline.cpp
)

set(UTILS_SOURCES
    src/utils/Logger.cpp
    src/utils/Config.cpp
)

set(ALL_SOURCES
    ${CORE_SOURCES}
    ${PROCESSING_SOURCES}
    ${UTILS_SOURCES}
    src/main.cpp
)

# Create main executable
add_executable(izana-backend ${ALL_SOURCES})

# Link libraries
target_link_libraries(izana-backend 
    ${SERIAL_LIBRARIES}
    Threads::Threads
)

# Handle JSON library linking
if(nlohmann_json_FOUND)
    target_link_libraries(izana-backend nlohmann_json::nlohmann_json)
else()
    target_include_directories(izana-backend PRIVATE ${JSON_INCLUDE_DIRS})
    target_link_libraries(izana-backend ${JSON_LIBRARIES})
endif()

# Compiler flags
target_include_directories(izana-backend PRIVATE ${SERIAL_INCLUDE_DIRS})
target_compile_options(izana-backend PRIVATE ${SERIAL_CFLAGS_OTHER})

# Set optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(izana-backend PRIVATE -O3 -DNDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(izana-backend PRIVATE -g -O0 -DDEBUG)
else()
    set(CMAKE_BUILD_TYPE "Release")
    target_compile_options(izana-backend PRIVATE -O3 -DNDEBUG)
endif()

# Build tests if requested
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Build examples if requested
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation
install(TARGETS izana-backend
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include/izana
    FILES_MATCHING PATTERN "*.hpp"
)

# Package configuration
set(CPACK_PACKAGE_NAME "proyecto-izana")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION "Modular Digital Oscilloscope Backend")
set(CPACK_PACKAGE_CONTACT "proyecto.izana@example.com")

include(CPack)