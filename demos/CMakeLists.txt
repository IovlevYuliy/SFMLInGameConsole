cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(SFMLInGameConsole)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

########################
####### IMGUI DEMO #####
########################

add_executable(imguiDemo imguiDemo.cpp
../include/QuakeStyleConsole.h
../include/IMGUIQuakeConsole.h
../include/ConsoleFormatting.h
3rdParty/imgui/imgui.cpp
3rdParty/imgui/imgui_widgets.cpp
3rdParty/imgui/imgui_draw.cpp
3rdParty/imgui/misc/cpp/imgui_stdlib.cpp
)

target_include_directories(imguiDemo PUBLIC "3rdParty")
target_include_directories(imguiDemo PUBLIC "3rdParty/imgui")
target_include_directories(imguiDemo PUBLIC "3rdParty/glfw/include")
target_include_directories(imguiDemo PUBLIC "3rdParty/glhpp")
target_include_directories(imguiDemo PRIVATE "../include")

target_link_libraries(imguiDemo OpenGL::GL)

if (${APPLE})
    target_link_libraries(imguiDemo "-framework Cocoa")
    target_link_libraries(imguiDemo "-framework IoKit")
endif()

add_subdirectory(3rdParty/glfw)
target_link_libraries(imguiDemo glfw)

########################
####### SFML DEMO  #####
########################

add_executable(sfmlDemo sfmlDemo.cpp)

file(GLOB SOURCES "../src/*")
target_sources(sfmlDemo PRIVATE ${SOURCES})

target_include_directories(sfmlDemo PRIVATE "../include")

add_subdirectory(3rdParty/SFML)
target_link_libraries(sfmlDemo PRIVATE sfml-graphics)
target_compile_features(sfmlDemo PRIVATE cxx_std_20)

add_custom_command(TARGET sfmlDemo POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                       ${CMAKE_SOURCE_DIR}/FreeMono.ttf $<TARGET_FILE_DIR:sfmlDemo>)

if(WIN32)
    add_custom_command(
        TARGET main
        COMMENT "Copy OpenAL DLL"
        PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/3rdParty/SFML/extlibs/bin/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/openal32.dll $<TARGET_FILE_DIR:main>
        VERBATIM)
endif()
