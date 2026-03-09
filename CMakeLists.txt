cmake_minimum_required(VERSION 3.15)

# Read version from VERSION.txt so it stays in sync with the release tag.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" PANTHEONCHAIN_VERSION_RAW)
string(STRIP "${PANTHEONCHAIN_VERSION_RAW}" PANTHEONCHAIN_VERSION)

project(PantheonChain VERSION ${PANTHEONCHAIN_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_library(pantheon_common STATIC
  common/crypto/cryptography.cpp
  common/p2p/p2p_network.cpp
  common/storage/storage.cpp
  common/serialization/serialization.cpp
  common/serialization/commitments.cpp
  common/mempool/mempool.cpp
  common/metrics/metrics.cpp
  common/monetary/denominations.cpp
  common/monetary/units.cpp
  common/utils/bridging.cpp
)
target_include_directories(pantheon_common PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/layer1-talanton/core)

add_library(pantheon_l1 STATIC layer1-talanton/tx/l1_commitment_validator.cpp)
target_include_directories(pantheon_l1 PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/layer1-talanton/core)
target_link_libraries(pantheon_l1 PUBLIC pantheon_common)

add_library(pantheon_l2 STATIC
  layer2-drachma/consensus/pos_consensus.cpp
  layer2-drachma/payments/payments_state_machine.cpp
)
target_include_directories(pantheon_l2 PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/layer1-talanton/core)
target_link_libraries(pantheon_l2 PUBLIC pantheon_common)

add_library(pantheon_l3 STATIC
  layer3-obolos/consensus/pos_consensus.cpp
  layer3-obolos/evm/execution.cpp
)
target_include_directories(pantheon_l3 PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/layer1-talanton/core)
target_link_libraries(pantheon_l3 PUBLIC pantheon_common)

# Bridge implementations (L1↔L2 and L2↔L3 canonical lock-mint/burn-unlock bridge)
add_library(pantheon_bridge STATIC
  bridge/l1_l2/l1_l2_bridge.cpp
  bridge/l2_l3/l2_l3_bridge.cpp
)
target_include_directories(pantheon_bridge PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(pantheon_bridge PUBLIC pantheon_common)

add_executable(pantheon-l1-talanton-node common/utils/pantheon_node.cpp)
add_executable(pantheon-l2-drachma-node common/utils/pantheon_node.cpp)
add_executable(pantheon-l3-obolos-node common/utils/pantheon_node.cpp)
target_compile_definitions(pantheon-l1-talanton-node PRIVATE PANTHEON_NODE_LAYER="l1")
target_compile_definitions(pantheon-l2-drachma-node PRIVATE PANTHEON_NODE_LAYER="l2")
target_compile_definitions(pantheon-l3-obolos-node PRIVATE PANTHEON_NODE_LAYER="l3")

# Canonical node binary aliases per the PantheonChain architecture spec.
# These names (pantheon_tlt_node, pantheon_drc_node, pantheon_obl_node) are
# the required public-facing binary names.  The *-node targets above are kept
# as the primary build targets to avoid renaming source references.
set_target_properties(pantheon-l1-talanton-node PROPERTIES OUTPUT_NAME "pantheon_tlt_node")
set_target_properties(pantheon-l2-drachma-node  PROPERTIES OUTPUT_NAME "pantheon_drc_node")
set_target_properties(pantheon-l3-obolos-node   PROPERTIES OUTPUT_NAME "pantheon_obl_node")

add_executable(pantheon-cli cli/pantheon-cli/main.cpp)
target_link_libraries(pantheon-cli PRIVATE pantheon_common)

add_executable(pantheon-relayer-l2 relayers/relayer-l2/main.cpp)
add_executable(pantheon-relayer-l3 relayers/relayer-l3/main.cpp)
target_link_libraries(pantheon-relayer-l2 PRIVATE pantheon_common pantheon_l1 pantheon_l2)
target_link_libraries(pantheon-relayer-l3 PRIVATE pantheon_common pantheon_l2)

# Third-party stubs (secp256k1 and leveldb) used by layer1-talanton
set(SECP256K1_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/stubs/secp256k1/include")
add_subdirectory(third_party/stubs/secp256k1)
add_subdirectory(third_party/stubs/leveldb)

# Compatibility shim: parthenon_monetary wraps the monetary code in pantheon_common
add_library(parthenon_monetary INTERFACE)
target_link_libraries(parthenon_monetary INTERFACE pantheon_common)

# Convenience aggregate: pantheon_layers bundles all layer libraries for tests/relayers
add_library(pantheon_layers INTERFACE)
target_link_libraries(pantheon_layers INTERFACE pantheon_common pantheon_l1 pantheon_l2 pantheon_l3 pantheon_bridge)

# Mobile SDK and other tools
add_subdirectory(tools)

# EVM library (Layer 3 Obolos execution engine)
add_subdirectory(layer3-obolos/evm)

# Layer 2 Drachma library (bridges, indexers, DEX, rollups, etc.)
add_subdirectory(layer2-drachma)

# Layer 1 full library set (parthenon_node, parthenon_wallet, parthenon_rpc, etc.)
add_subdirectory(layer1-talanton)

# pantheon_common uses parthenon::crypto::SHA256d::Hash256d from parthenon_crypto
target_link_libraries(pantheon_common PUBLIC parthenon_crypto)

# Client applications: parthenond (daemon), parthenon-cli, parthenon-qt (desktop wallet)
add_subdirectory(clients)

# Test suite
add_subdirectory(tests)

# ------------------------------------------------------------------ #
#  Install rules                                                       #
# ------------------------------------------------------------------ #
include(GNUInstallDirs)

# Node binaries (output as pantheon_tlt_node / pantheon_drc_node / pantheon_obl_node)
install(PROGRAMS
    $<TARGET_FILE:pantheon-l1-talanton-node>
    $<TARGET_FILE:pantheon-l2-drachma-node>
    $<TARGET_FILE:pantheon-l3-obolos-node>
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT runtime
)

# Daemon binary (parthenond)
install(PROGRAMS
    $<TARGET_FILE:parthenond>
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT runtime
)

# CLI
install(PROGRAMS
    $<TARGET_FILE:pantheon-cli>
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT runtime
)

# Relayers
install(PROGRAMS
    $<TARGET_FILE:pantheon-relayer-l2>
    $<TARGET_FILE:pantheon-relayer-l3>
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT runtime
)

# Default configuration
install(FILES
    parthenond.conf.example
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/pantheon
    RENAME parthenond.conf
    COMPONENT config
)

install(DIRECTORY
    configs/devnet configs/testnet configs/mainnet
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/pantheon
    COMPONENT config
)

install(FILES
    genesis_talanton.json genesis_drachma.json genesis_obolos.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pantheon
    COMPONENT config
)

# Documentation
install(FILES
    README.md CHANGELOG.md LICENSE WHITEPAPER.md EULA.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
    COMPONENT docs
)

# ------------------------------------------------------------------ #
#  CPack packaging                                                     #
# ------------------------------------------------------------------ #
set(CPACK_PACKAGE_NAME            "pantheonchain")
set(CPACK_PACKAGE_VENDOR          "PantheonChain Foundation")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "PantheonChain — three-layer PoW/PoS blockchain with EVM smart contracts")
set(CPACK_PACKAGE_VERSION         "${PANTHEONCHAIN_VERSION}")
set(CPACK_PACKAGE_CONTACT         "contact@pantheonchain.org")
set(CPACK_PACKAGE_HOMEPAGE_URL    "https://pantheonchain.org")
set(CPACK_RESOURCE_FILE_LICENSE   "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README    "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_STRIP_FILES             ON)
set(CPACK_COMPONENTS_ALL          runtime config docs)

# DEB-specific
set(CPACK_DEBIAN_PACKAGE_NAME     "pantheonchain")
set(CPACK_DEBIAN_PACKAGE_SECTION  "net")
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libssl3 (>= 3.0.0), libboost-system1.74.0 | libboost-system1.83.0, \
libboost-filesystem1.74.0 | libboost-filesystem1.83.0, \
libboost-thread1.74.0 | libboost-thread1.83.0")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "curl")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "PantheonChain Foundation <contact@pantheonchain.org>")

# RPM-specific
set(CPACK_RPM_PACKAGE_NAME        "pantheonchain")
set(CPACK_RPM_PACKAGE_LICENSE     "MIT")
set(CPACK_RPM_PACKAGE_GROUP       "Applications/Internet")
set(CPACK_RPM_PACKAGE_REQUIRES    "openssl >= 3.0.0, boost >= 1.74.0")

include(CPack)
