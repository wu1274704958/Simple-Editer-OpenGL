cmake_minimum_required(VERSION 3.0.0)
project(OpenGL_1 VERSION 0.1.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
if(WIN32)

    add_compile_options("/std:c++17")
    set(CMAKE_EXE_LINKER_FLAGS "/SAFESEH:NO /subsystem:windows /entry:mainCRTStartup")

else()


endif()

find_package(GLM)
find_package(GLFW3)

if(GLM_FOUND)
    message(${GLM_INCLUDE_DIR})
endif()

include_directories(D:/MyLib/glfw-3.2.1/include
                    . 
                    "D:/MyLib/Simple OpenGL Image Library/src"
                    D:/MyLib/glm-0.9.6.3/glm)

link_directories(   D:/MyLib/glfw-3.2.1/build/src/Release 
                   "D:/MyLib/Simple OpenGL Image Library/projects/VC9/Release"
                  )           

add_executable(OpenGL_1 main.cpp RenderDemo.cpp glad.c)

target_link_libraries(OpenGL_1 glfw3 SOIL Opengl32.lib)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
