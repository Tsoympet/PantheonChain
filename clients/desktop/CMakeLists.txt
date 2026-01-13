cmake_minimum_required(VERSION 3.15)

# Desktop wallet - Full Qt implementation

# Find Qt5
find_package(Qt5 COMPONENTS Widgets Network QUIET)

if(Qt5_FOUND)
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
    target_link_libraries(parthenon-qt Qt5::Widgets Qt5::Network)
    
    message(STATUS "Building ParthenonChain Desktop Wallet with Qt ${Qt5_VERSION}")
else()
    # Fallback to scaffold version
    message(WARNING "Qt5 not found. Building scaffold version.")
    add_executable(parthenon-qt
        gui/main.cpp
    )
    target_compile_features(parthenon-qt PRIVATE cxx_std_17)
endif()

install(TARGETS parthenon-qt DESTINATION bin)
