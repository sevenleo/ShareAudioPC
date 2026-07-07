cmake_minimum_required(VERSION 3.24)

project(ShareAudioLite
    VERSION 0.1.0
    DESCRIPTION "Cross-platform low-latency LAN audio transmitter and receiver"
    LANGUAGES CXX
)

find_package(Threads REQUIRED)

option(SHAREAUDIO_ENABLE_OPUS "Enable libopus-backed Quality Mode codec support" OFF)
option(SHAREAUDIO_ENABLE_MINIAUDIO "Enable miniaudio-backed real audio capture/playback" ON)
option(SHAREAUDIO_ENABLE_TESTS "Build automated tests" ON)
option(SHAREAUDIO_ENABLE_CONSOLE_UI "Build the console UI executable" ON)
option(SHAREAUDIO_ENABLE_DESKTOP_UI "Build a future desktop UI target" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(shareaudio_core
    src/app/Application.cpp
    src/app/AppController.cpp
    src/app/Config.cpp
    src/app/Logger.cpp
    src/app/SessionController.cpp
    src/audio/AudioAbstractions.cpp
    src/audio/MiniaudioBackend.cpp
    src/audio/AudioPipeline.cpp
    src/codec/OpusCodec.cpp
    src/network/LoopbackTest.cpp
    src/network/PcmBroadcastServer.cpp
    src/network/TcpSocket.cpp
    src/platform/LocalIp.cpp
    src/protocol/JitterBuffer.cpp
    src/protocol/PcmChunker.cpp
    src/protocol/Protocol.cpp
    src/storage/RecentDevices.cpp
    src/ui/ConsoleUi.cpp
)

target_include_directories(shareaudio_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio-src/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniaudio-src
)

target_compile_definitions(shareaudio_core
    PUBLIC
        ASIO_STANDALONE=1
        SHAREAUDIO_VERSION="${PROJECT_VERSION}"
        SHAREAUDIO_ENABLE_OPUS=$<BOOL:${SHAREAUDIO_ENABLE_OPUS}>
        SHAREAUDIO_ENABLE_MINIAUDIO=$<BOOL:${SHAREAUDIO_ENABLE_MINIAUDIO}>
        SHAREAUDIO_ENABLE_CONSOLE_UI=$<BOOL:${SHAREAUDIO_ENABLE_CONSOLE_UI}>
        SHAREAUDIO_ENABLE_DESKTOP_UI=$<BOOL:${SHAREAUDIO_ENABLE_DESKTOP_UI}>
)

if(MSVC)
    target_compile_options(shareaudio_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(shareaudio_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    target_link_libraries(shareaudio_core PUBLIC ws2_32 mswsock ole32 uuid winmm avrt)
else()
    target_link_libraries(shareaudio_core PUBLIC dl m)
endif()

target_link_libraries(shareaudio_core PUBLIC Threads::Threads)

if(SHAREAUDIO_ENABLE_OPUS)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(OPUS QUIET opus)
    endif()

    if(OPUS_FOUND)
        target_include_directories(shareaudio_core PUBLIC ${OPUS_INCLUDE_DIRS})
        target_link_libraries(shareaudio_core PUBLIC ${OPUS_LIBRARIES})
        target_compile_definitions(shareaudio_core PUBLIC SHAREAUDIO_HAS_LIBOPUS=1)
    else()
        message(WARNING "SHAREAUDIO_ENABLE_OPUS is ON, but libopus was not found. The codec wrapper will report NotSupported at runtime.")
        target_compile_definitions(shareaudio_core PUBLIC SHAREAUDIO_HAS_LIBOPUS=0)
    endif()
else()
    target_compile_definitions(shareaudio_core PUBLIC SHAREAUDIO_HAS_LIBOPUS=0)
endif()

if(SHAREAUDIO_ENABLE_CONSOLE_UI)
    add_executable(shareaudio_cli src/main.cpp)
    target_link_libraries(shareaudio_cli PRIVATE shareaudio_core)
endif()

if(SHAREAUDIO_ENABLE_DESKTOP_UI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
    add_executable(shareaudio_gui
        src/gui/main.cpp
        src/gui/MainWindow.cpp
    )
    target_link_libraries(shareaudio_gui PRIVATE shareaudio_core Qt6::Widgets)
    if(WIN32)
        set_target_properties(shareaudio_gui PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()

if(SHAREAUDIO_ENABLE_TESTS)
    enable_testing()
    add_executable(shareaudio_tests
        tests/main.cpp
    )
    target_link_libraries(shareaudio_tests PRIVATE shareaudio_core)
    add_test(NAME shareaudio_tests COMMAND shareaudio_tests)
endif()
