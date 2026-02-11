cmake_minimum_required(VERSION 3.15)

# Desktop wallet - Full Qt implementation

option(PARTHENON_DESKTOP_REQUIRE_QT "Fail configuration when no supported Qt version is available" OFF)

# Prefer Qt6, then fallback to Qt5.
find_package(Qt6 COMPONENTS Widgets Network QUIET)
if(Qt6_FOUND)
    set(PARTHENON_QT_TARGET_PREFIX Qt6)
    set(PARTHENON_QT_VERSION "${Qt6_VERSION}")
else()
    find_package(Qt5 COMPONENTS Widgets Network QUIET)
    if(Qt5_FOUND)
        set(PARTHENON_QT_TARGET_PREFIX Qt5)
        set(PARTHENON_QT_VERSION "${Qt5_VERSION}")
    endif()
endif()

if(DEFINED PARTHENON_QT_TARGET_PREFIX)
    # Full Qt implementation
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    add_executable(parthenon-qt
        src/main.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/rpc_client.cpp
        src/rpc_client.h
        src/overviewpage.cpp
        src/overviewpage.h
        src/sendpage.cpp
        src/sendpage.h
        src/receivepage.cpp
        src/receivepage.h
        src/transactionpage.cpp
        src/transactionpage.h
    )

    target_compile_features(parthenon-qt PRIVATE cxx_std_17)
    target_link_libraries(parthenon-qt
        ${PARTHENON_QT_TARGET_PREFIX}::Widgets
        ${PARTHENON_QT_TARGET_PREFIX}::Network
    )

    message(STATUS "Building ParthenonChain Desktop Wallet with ${PARTHENON_QT_TARGET_PREFIX} ${PARTHENON_QT_VERSION}")
else()
    if(PARTHENON_DESKTOP_REQUIRE_QT)
        message(FATAL_ERROR
            "Neither Qt6 nor Qt5 was found. Install Qt development packages or disable PARTHENON_DESKTOP_REQUIRE_QT."
        )
    endif()

    # Fallback to scaffold version for environments without Qt.
    message(STATUS "Qt not found (Qt6/Qt5). Building desktop scaffold target.")
    add_executable(parthenon-qt
        gui/main.cpp
    )
    target_compile_features(parthenon-qt PRIVATE cxx_std_17)
endif()

install(TARGETS parthenon-qt DESTINATION bin)
