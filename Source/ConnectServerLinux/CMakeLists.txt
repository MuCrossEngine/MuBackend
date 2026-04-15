cmake_minimum_required(VERSION 3.20)

project(ConnectServerLinux LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CONNECTSERVER_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../Archives/1.- ConnectServer")

add_executable(connectserver
    ConnectServer/CGMHardwareId.cpp
    ConnectServer/ClientManager.cpp
    ConnectServer/ConnectServer.cpp
    ConnectServer/ConnectServerProtocol.cpp
    ConnectServer/ConnectVersionHex.cpp
    ConnectServer/CriticalSection.cpp
    ConnectServer/IpManager.cpp
    ConnectServer/Log.cpp
    ConnectServer/MemScript.cpp
    ConnectServer/MiniDump.cpp
    ConnectServer/Protect.cpp
    ConnectServer/Queue.cpp
    ConnectServer/RuntimeContext.cpp
    ConnectServer/ServerDisplayer.cpp
    ConnectServer/ServerList.cpp
    ConnectServer/SocketManager.cpp
    ConnectServer/SocketManagerUdp.cpp
    ConnectServer/Util.cpp
    ConnectServer/stdafx.cpp
)

target_include_directories(connectserver PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ConnectServer")
target_compile_definitions(connectserver PRIVATE CONNECTSERVER_UPDATE=803 PROTECT_STATE=0)

if(UNIX)
    target_compile_options(connectserver PRIVATE -fpermissive)
endif()

find_package(Threads REQUIRED)
target_link_libraries(connectserver PRIVATE Threads::Threads)

set_target_properties(connectserver PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CONNECTSERVER_OUTPUT_DIR}"
    OUTPUT_NAME "connectserver"
)
