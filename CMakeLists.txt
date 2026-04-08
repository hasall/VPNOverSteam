cmake_minimum_required(VERSION 3.15)
project(VPNOverSteam)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Add include directories
include_directories(include)

# Create library
add_library(VPNOverSteamLib SHARED 
    src/lib/Client.cpp
    src/lib/DebugLog.cpp
    src/lib/IpPool.cpp
    src/lib/LobbyClient.cpp
    src/lib/LobbyClientController.cpp
    src/lib/LobbyServer.cpp
    src/lib/LobbyServerController.cpp
    src/lib/Server.cpp
    src/lib/SteamLoop.cpp
    src/lib/SteamMessageProcessor.cpp
    src/lib/TUNLinux.cpp
    src/lib/TUNMock.cpp
    src/lib/TUNWindows.cpp
    src/lib/Utils.cpp
    )

target_compile_definitions(VPNOverSteamLib PRIVATE VPN_OVER_STEAM_EXPORTS)

# Create executable
add_executable(VPNOverSteamApp 
    src/app/AutomaticMain.cpp
    src/app/ManualMain.cpp
    src/app/Main.cpp)

# Link library
target_link_libraries(VPNOverSteamApp PRIVATE VPNOverSteamLib)

# Link Steam SDK (assuming it's located in "third_party/steam_sdk")
set(STEAM_ROOT ${CMAKE_SOURCE_DIR}/third_party/steam)
include_directories(${STEAM_ROOT}/include)

# Copy Steam app id file to output directory so runtime finds it
add_custom_command(TARGET VPNOverSteamApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${STEAM_ROOT}/steam_appid.txt
        $<TARGET_FILE_DIR:VPNOverSteamApp>
)

# Platform-specific settings
if(WIN32)
    message(STATUS "Building for Windows")

    # Link Steam API
    target_link_libraries(VPNOverSteamLib PRIVATE "${STEAM_ROOT}/lib/win64/steam_api64.lib")

    # Link Windows networking / crypto libs used by project
    target_link_libraries(VPNOverSteamLib PRIVATE ws2_32 iphlpapi bcrypt)

    # App also uses Steam API symbols directly (via steam callbacks in main path)
    target_link_libraries(VPNOverSteamApp PRIVATE "${STEAM_ROOT}/lib/win64/steam_api64.lib")

    # Copy Steam DLL to output directory so runtime finds it
    add_custom_command(TARGET VPNOverSteamApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${STEAM_ROOT}/lib/win64/steam_api64.dll
            $<TARGET_FILE_DIR:VPNOverSteamApp>
    )
    add_custom_command(TARGET VPNOverSteamLib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${STEAM_ROOT}/lib/win64/steam_api64.dll
            $<TARGET_FILE_DIR:VPNOverSteamLib>
    )

    # Link Wintun (assuming it's located in "third_party/wintun")
    set(WINTUN_ROOT ${CMAKE_SOURCE_DIR}/third_party/wintun)
    include_directories(${WINTUN_ROOT}/include)
    
    # Copy Wintun DLL to output directory so runtime finds it
    add_custom_command(TARGET VPNOverSteamApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${WINTUN_ROOT}/bin/win64/wintun.dll
            $<TARGET_FILE_DIR:VPNOverSteamApp>
    )
    add_custom_command(TARGET VPNOverSteamLib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${WINTUN_ROOT}/bin/win64/wintun.dll
            $<TARGET_FILE_DIR:VPNOverSteamLib>
    )
elseif(UNIX)
    message(STATUS "Building for Linux")
    
    # Link Steam API
    target_link_libraries(VPNOverSteamLib PRIVATE "${STEAM_ROOT}/lib/linux64/libsteam_api.so")
    target_link_libraries(VPNOverSteamApp PRIVATE "${STEAM_ROOT}/lib/linux64/libsteam_api.so")
    
    add_custom_command(TARGET VPNOverSteamLib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${STEAM_ROOT}/lib/linux64/steamclient.so
            $<TARGET_FILE_DIR:VPNOverSteamLib>
    )

    # Find and link OpenSSL
    find_package(OpenSSL REQUIRED)
    target_link_libraries(VPNOverSteamLib PRIVATE OpenSSL::SSL OpenSSL::Crypto)


endif()