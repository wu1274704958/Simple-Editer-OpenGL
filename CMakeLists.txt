cmake_minimum_required(VERSION 3.0.0)
project(Simple_Editer VERSION 0.1.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
if(WIN32)

    message("is Windows")
    add_definitions(/permissive-)
    add_link_options(/SAFESEH:NO /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /NODEFAULTLIB:library)

else()


endif()

find_package(GLM)
find_package(GLFW3)

if(GLM_FOUND AND GLFW3_FOUND)
    message(${GLM_INCLUDE_DIR})

    include_directories( ${GLFW3_INCLUDE_DIR}
    . 
    ${CMAKE_SOURCE_DIR}/3rd_party/stb
    ${GLM_INCLUDE_DIR} )

    add_executable(${PROJECT_NAME} main.cpp RenderDemo.cpp glad.c)

    target_link_libraries(${PROJECT_NAME}  ${GLFW3_LIBRARY}   )

    set(CPACK_PROJECT_NAME ${PROJECT_NAME})
    set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

endif()

        
