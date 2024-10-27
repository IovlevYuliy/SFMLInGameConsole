cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(VirtuosoConsole)

add_executable(ConsoleTest consoleTest.cpp ../include/QuakeStyleConsole.h)

add_custom_command(TARGET ConsoleTest POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                       ${CMAKE_SOURCE_DIR}/TestCommands.txt $<TARGET_FILE_DIR:ConsoleTest>)

add_custom_command(TARGET ConsoleTest POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                       ${CMAKE_SOURCE_DIR}/file2.txt $<TARGET_FILE_DIR:ConsoleTest>)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

########################
####### IMGUI DEMO #####
########################

add_executable(imguiDemo imguiDemo.cpp
../include/QuakeStyleConsole.h
../include/IMGUIQuakeConsole.h
../include/ConsoleFormatting.h
Depends/imgui/imgui.cpp
Depends/imgui/imgui_widgets.cpp
Depends/imgui/imgui_draw.cpp
Depends/imgui/misc/cpp/imgui_stdlib.cpp
)

target_include_directories(imguiDemo PUBLIC "Depends")
target_include_directories(imguiDemo PUBLIC "Depends/imgui")
target_include_directories(imguiDemo PUBLIC "Depends/glfw/include")
target_include_directories(imguiDemo PUBLIC "Depends/glhpp")
target_include_directories(imguiDemo PRIVATE "../include")

target_link_libraries(imguiDemo OpenGL::GL)

if (${APPLE})
    target_link_libraries(imguiDemo "-framework Cocoa")
    target_link_libraries(imguiDemo "-framework IoKit")
endif()

add_subdirectory(Depends/glfw)
target_link_libraries(imguiDemo glfw)

########################
####### SFML DEMO  #####
########################

add_executable(sfmlDemo sfmlDemo.cpp)

file(GLOB SOURCES "../src/*")
target_sources(sfmlDemo PRIVATE ${SOURCES})

target_include_directories(sfmlDemo PRIVATE "../include")

add_subdirectory(Depends/SFML)
target_link_libraries(sfmlDemo PRIVATE sfml-graphics)
target_compile_features(sfmlDemo PRIVATE cxx_std_20)

add_custom_command(TARGET sfmlDemo POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                       ${CMAKE_SOURCE_DIR}/FreeMono.ttf $<TARGET_FILE_DIR:sfmlDemo>)

if(WIN32)
    add_custom_command(
        TARGET main
        COMMENT "Copy OpenAL DLL"
        PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/Depends/SFML/extlibs/bin/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/openal32.dll $<TARGET_FILE_DIR:main>
        VERBATIM)
endif()
