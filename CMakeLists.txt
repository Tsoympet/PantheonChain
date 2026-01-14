cmake_minimum_required(VERSION 3.15)
project(ParthenonChain VERSION 1.0.0 LANGUAGES CXX C)

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
# Remove strict warning flags from secp256k1 target to avoid errors in third-party code
if(TARGET secp256k1)
  if(MSVC)
    # MSVC: Disable warnings as errors
    target_compile_options(secp256k1 PRIVATE /WX-)
  else()
    # GCC/Clang: Disable warnings as errors
    target_compile_options(secp256k1 PRIVATE -Wno-error)
  endif()
endif()

# Build LevelDB for blockchain storage (without strict warnings)
set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "Build LevelDB tests")
set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "Build LevelDB benchmarks")
add_subdirectory(third_party/leveldb EXCLUDE_FROM_ALL)
# Remove strict warning flags from leveldb target to avoid errors in third-party code
if(TARGET leveldb)
  if(MSVC)
    # MSVC: Disable warnings as errors and suppress CRT security warnings for third-party code
    target_compile_options(leveldb PRIVATE /WX-)
    target_compile_definitions(leveldb PRIVATE _CRT_SECURE_NO_WARNINGS)
  else()
    # GCC/Clang: Disable specific warnings
    target_compile_options(leveldb PRIVATE -Wno-error -Wno-conversion -Wno-old-style-cast -Wno-shadow)
  endif()
endif()

# Include directories for header-only libraries (as system includes to suppress warnings)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/third_party/cpp-httplib)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/third_party/json/include)

# Include directory for project headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Layer 1 Core - Consensus Critical
add_subdirectory(layer1)

# Layer 2 - Non-Consensus Modules
add_subdirectory(layer2)

# Client Applications
add_subdirectory(clients)

# Tests
add_subdirectory(tests)

# CPack configuration for package generation
set(CPACK_PACKAGE_NAME "ParthenonChain")
set(CPACK_PACKAGE_VENDOR "ParthenonChain Foundation")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ParthenonChain - A Multi-Asset Proof-of-Work Blockchain with Smart Contracts")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "ParthenonChain")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# Platform-specific packaging
if(WIN32)
  set(CPACK_GENERATOR "NSIS;ZIP")
  set(CPACK_NSIS_DISPLAY_NAME "ParthenonChain ${PROJECT_VERSION}")
  set(CPACK_NSIS_PACKAGE_NAME "ParthenonChain")
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
  set(CPACK_NSIS_MODIFY_PATH ON)
elseif(APPLE)
  set(CPACK_GENERATOR "DragNDrop;TGZ")
  set(CPACK_DMG_VOLUME_NAME "ParthenonChain")
  set(CPACK_DMG_FORMAT "UDZO")
elseif(UNIX)
  set(CPACK_GENERATOR "DEB;RPM;TGZ")
  
  # Debian package configuration
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ParthenonChain Foundation <dev@parthenonchain.org>")
  set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
  set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libssl3 | libssl1.1, libboost-system1.74.0 | libboost-system1.71.0")
  
  # RPM package configuration
  set(CPACK_RPM_PACKAGE_LICENSE "MIT")
  set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
  set(CPACK_RPM_PACKAGE_REQUIRES "openssl, boost-system")
endif()

include(CPack)
