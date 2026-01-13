cmake_minimum_required(VERSION 3.15)
project(ParthenonChain VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for security and performance (save original)
set(ORIGINAL_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
endif()

# Enable testing
enable_testing()

# Build secp256k1 library
set(SECP256K1_BUILD_BENCHMARK OFF CACHE BOOL "Build secp256k1 benchmarks")
set(SECP256K1_BUILD_TESTS OFF CACHE BOOL "Build secp256k1 tests")
set(SECP256K1_BUILD_EXHAUSTIVE_TESTS OFF CACHE BOOL "Build exhaustive tests")
set(SECP256K1_ENABLE_MODULE_SCHNORRSIG ON CACHE BOOL "Enable Schnorr signature module")
set(SECP256K1_ENABLE_MODULE_EXTRAKEYS ON CACHE BOOL "Enable extra keys module")
add_subdirectory(third_party/secp256k1 EXCLUDE_FROM_ALL)

# Build LevelDB for blockchain storage (without -Werror)
if(MSVC)
  set(CMAKE_CXX_FLAGS "${ORIGINAL_CXX_FLAGS} /W3")
else()
  set(CMAKE_CXX_FLAGS "${ORIGINAL_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
endif()
set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "Build LevelDB tests")
set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "Build LevelDB benchmarks")
add_subdirectory(third_party/leveldb EXCLUDE_FROM_ALL)
# Restore flags
if(MSVC)
  set(CMAKE_CXX_FLAGS "${ORIGINAL_CXX_FLAGS} /W4 /WX")
else()
  set(CMAKE_CXX_FLAGS "${ORIGINAL_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
endif()

# Include directories for header-only libraries
include_directories(${CMAKE_SOURCE_DIR}/third_party/cpp-httplib)
include_directories(${CMAKE_SOURCE_DIR}/third_party/json/include)

# Layer 1 Core - Consensus Critical
add_subdirectory(layer1)

# Layer 2 - Non-Consensus Modules
add_subdirectory(layer2)

# Client Applications
add_subdirectory(clients)

# Tests
add_subdirectory(tests)
