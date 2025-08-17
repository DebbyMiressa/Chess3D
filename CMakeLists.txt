#Template for INFOH502
#Heavily inspired from https://github.com/Polytonic/Glitter/blob/master/CMakeLists.txt

# set the minimal version of cmake first (must precede project())
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# project setup
project(infoh502-cpp VERSION 0.1)

# set the minimal version of the c++ language
set(CMAKE_CXX_STANDARD 14)     
set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(OpenGL REQUIRED)

#for glad library
add_library( glad STATIC 3rdParty/glad/src/glad.c)
set(GLAD_INCLUDE "3rdParty/glad/include")
target_include_directories(glad PUBLIC ${GLAD_INCLUDE})

#for stb_image library (header-only, implementation defined in Cubemap.cpp)

#for GLFW
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)

#Search for glfw in case it is installed
find_package(glfw3 QUIET)
if (NOT glfw3_FOUND)
    #if glfw is not found, we will use the sources from the submodules
    add_subdirectory(3rdParty/glfw)
    include_directories(3rdParty/glfw/include/)
endif()

include_directories(3rdParty/glad/include/
                    3rdParty/glm/
                    3rdParty/stb/)

# Project: Chess (merged into top-level)

# Source files
set(CHESS_SRC
    Chess/src/main.cpp
    Chess/src/camera.h
    Chess/src/shader.h
    Chess/src/Object/Mesh.h
    # Lighting and Reflection feature
    Chess/src/Feature/basic/LightingAndReflection/LightingAndReflection.h
    Chess/src/Feature/basic/LightingAndReflection/LightingAndReflection.cpp
    # LoadModel feature
    Chess/src/Feature/basic/LoadModel/LoadModel.h
    Chess/src/Feature/basic/LoadModel/LoadModel.cpp
    # CameraControl feature
    Chess/src/Feature/basic/CameraControl/CameraControl.h
    Chess/src/Feature/basic/CameraControl/CameraControl.cpp
    # GameLogic feature
    Chess/src/Feature/basic/GameLogic/Types.h
    Chess/src/Feature/basic/GameLogic/GameLogic.h
    Chess/src/Feature/basic/GameLogic/GameLogic.cpp
    # MoveObject feature
    Chess/src/Feature/basic/MoveObject/MoveObject.h
    Chess/src/Feature/basic/MoveObject/MoveObject.cpp
    # Texture feature
    Chess/src/Feature/basic/Texture/Texture.h
    Chess/src/Feature/basic/Texture/Texture.cpp
    # Cubemap feature
    Chess/src/Feature/basic/Cubemap/Cubemap.h
    Chess/src/Feature/basic/Cubemap/Cubemap.cpp
    # Shadow feature (advanced)
    Chess/src/Feature/advanced/Shadow/Shadow.h
    Chess/src/Feature/advanced/Shadow/Shadow.cpp
    # Billboarding feature (intermediate)
    Chess/src/Feature/intermediate/Billboarding/Billboarding.h
    Chess/src/Feature/intermediate/Billboarding/Billboarding.cpp
)

add_executable(Chess ${CHESS_SRC})

# Compile-time definitions for asset paths
target_compile_definitions(Chess PRIVATE PATH_TO_OBJECTS="${CMAKE_SOURCE_DIR}/Chess/src/Object")
target_compile_definitions(Chess PRIVATE PATH_TO_SRC="${CMAKE_SOURCE_DIR}/Chess/src")

target_include_directories(Chess PUBLIC ${CMAKE_SOURCE_DIR}/Chess/src)
target_link_libraries(Chess PUBLIC OpenGL::GL glfw glad)

# Optional: build docs from markdown to PDF if pandoc is available
find_program(PANDOC_EXECUTABLE pandoc)
if(PANDOC_EXECUTABLE)
    set(REPORT_MD ${CMAKE_SOURCE_DIR}/docs/Project_Report.md)
    set(REPORT_PDF ${CMAKE_SOURCE_DIR}/docs/Project_Report.pdf)
    if(EXISTS ${REPORT_MD})
        add_custom_command(
            OUTPUT ${REPORT_PDF}
            COMMAND ${PANDOC_EXECUTABLE} -V geometry:margin=1in -V fontsize=11pt -o ${REPORT_PDF} ${REPORT_MD}
            DEPENDS ${REPORT_MD}
            COMMENT "Generating Project_Report.pdf from markdown"
            VERBATIM
        )
        add_custom_target(report ALL DEPENDS ${REPORT_PDF})
    endif()
endif()