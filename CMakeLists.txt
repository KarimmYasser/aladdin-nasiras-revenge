cmake_minimum_required(VERSION 3.5)   # Selects the minimum version of CMake required to run this file
project(GFX-LAB VERSION 0.1.0)          # Here we select the project name and version

# Here we select C++17 with all the standards required and all compiler-specific extensions disabled
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# These are the options we select for building GLFW as a library
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)        # Don't build Documentation
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)       # Don't build Tests
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)    # Don't build Examples
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)           # Don't build Installation Information
set(GLFW_USE_HYBRID_HPG ON CACHE BOOL "" FORCE)     # Add variables to use High Performance Graphics Card if available
add_subdirectory(vendor/glfw)                       # Build the GLFW project to use later as a library


set(RP3D_COMPILE_TESTBED OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/reactphysics3d)

# Assimp – disable everything we don't need to keep build times short
set(ASSIMP_BUILD_TESTS        OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT          ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_COLLADA_IMPORTER        ON  CACHE BOOL "" FORCE)  # .dae
set(ASSIMP_BUILD_FBX_IMPORTER           ON  CACHE BOOL "" FORCE)  # .fbx
set(ASSIMP_BUILD_OBJ_IMPORTER           ON  CACHE BOOL "" FORCE)  # .obj
set(ASSIMP_INSTALL            OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/assimp)

# A variable with all the source files of GLAD
set(GLAD_SOURCE vendor/glad/src/gl.c)
# A variables with all the source files of Dear ImGui
set(IMGUI_SOURCES
        vendor/imgui/imgui.cpp
        vendor/imgui/imgui_demo.cpp
        vendor/imgui/imgui_draw.cpp
        vendor/imgui/imgui_widgets.cpp
        vendor/imgui/imgui_impl/imgui_impl_glfw.cpp
        vendor/imgui/imgui_impl/imgui_impl_opengl3.cpp
        )

# Combine all vendor source files together into a single variable
set(VENDOR_SOURCES ${GLAD_SOURCE} ${IMGUI_SOURCES})

# A variable with all our source files that are common between executable targets (examples)
set(COMMON_SOURCES
        source/common/application.hpp
        source/common/application.cpp
        source/common/input/keyboard.hpp
        source/common/input/mouse.hpp

        source/common/asset-loader.cpp
        source/common/asset-loader.hpp
        source/common/deserialize-utils.hpp

        source/common/shader/shader.hpp
        source/common/shader/shader.cpp

        source/common/mesh/vertex.hpp
        source/common/mesh/mesh.hpp
        source/common/mesh/mesh-utils.hpp
        source/common/mesh/mesh-utils.cpp
        source/common/mesh/mesh.cpp
        source/common/mesh/skinned-vertex.hpp
        source/common/mesh/bone-info.hpp
        source/common/mesh/skinned-mesh.hpp
        source/common/mesh/skinned-mesh.cpp
        source/common/animation/animation-types.hpp
        source/common/animation/animation-loader.hpp
        source/common/animation/animation-loader.cpp
        source/common/animation/animator.hpp
        source/common/animation/animator.cpp

        source/common/texture/sampler.hpp
        source/common/texture/sampler.cpp
        source/common/texture/texture2d.hpp
        source/common/texture/texture-utils.hpp
        source/common/texture/texture-utils.cpp
        source/common/texture/screenshot.hpp
        source/common/texture/screenshot.cpp

        source/common/material/pipeline-state.hpp
        source/common/material/pipeline-state.cpp
        source/common/material/material.hpp
        source/common/material/material.cpp
        source/common/material/lit-material.hpp
        source/common/material/lit-material.cpp

        source/common/ecs/component.hpp
        source/common/ecs/transform.hpp
        source/common/ecs/transform.cpp
        source/common/ecs/entity.hpp
        source/common/ecs/entity.cpp
        source/common/ecs/world.hpp
        source/common/ecs/world.cpp

        source/common/components/camera.hpp
        source/common/components/camera.cpp
        source/common/components/mesh-renderer.hpp
        source/common/components/mesh-renderer.cpp
        source/common/components/free-camera-controller.hpp
        source/common/components/free-camera-controller.cpp
        source/common/components/movement.hpp
        source/common/components/movement.cpp
        source/common/components/aladdin-controller.hpp
        source/common/components/collectible.hpp
        source/common/components/light.hpp
        source/common/components/light.cpp
        source/common/components/component-deserializer.hpp
        source/common/components/animator-component.hpp

        source/common/systems/forward-renderer.hpp
        source/common/systems/forward-renderer.cpp
        source/common/systems/animation-system.hpp
        source/common/systems/free-camera-controller.hpp
        source/common/systems/movement.hpp
        source/common/systems/aladdin-controller.hpp
        source/common/systems/collectible.hpp

        source/common/audio/audio-system.hpp
        source/common/audio/audio-system.cpp

        source/common/video/mp4-decoder.hpp
        source/common/video/mp4-decoder.cpp
        source/common/video/video-cutscene-overlay.hpp
        source/common/video/video-cutscene-overlay.cpp
)

# Define the directories in which to search for the included headers
include_directories(
        source/common
        vendor/glfw/include
        vendor/glad/include
        vendor/glm
        vendor/imgui
        vendor/utils
        vendor/reactphysics3d/include
        vendor/miniaudio
        vendor/assimp/include
        vendor/ffmpeg/include
)

# --- FFmpeg (prebuilt MSVC shared: vendor/ffmpeg/{include,lib,bin}) ---
set(FFMPEG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor/ffmpeg" CACHE PATH "FFmpeg dev root (include/, lib/*.def, bin/*.dll)")
if(NOT EXISTS "${FFMPEG_ROOT}/include/libavformat/avformat.h")
    message(FATAL_ERROR "FFmpeg headers not found under ${FFMPEG_ROOT}/include. Add a MSVC shared dev build (include+lib+bin) to vendor/ffmpeg.")
endif()

set(FFMPEG_IMP_DIR "${CMAKE_BINARY_DIR}/ffmpeg_import_libs")
file(MAKE_DIRECTORY "${FFMPEG_IMP_DIR}")
file(GLOB FFMPEG_DEF_FILES "${FFMPEG_ROOT}/lib/*.def")
set(FFMPEG_IMPORT_LIBS "")
if(MSVC)
    get_filename_component(_VC_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(LIB_EXE "${_VC_BIN_DIR}/lib.exe")
    if(NOT EXISTS "${LIB_EXE}")
        message(FATAL_ERROR "Could not find lib.exe next to ${CMAKE_CXX_COMPILER}. FFmpeg import libs require the MSVC toolchain.")
    endif()
    foreach(_def ${FFMPEG_DEF_FILES})
        get_filename_component(_base "${_def}" NAME_WE)
        set(_implib "${FFMPEG_IMP_DIR}/${_base}.lib")
        execute_process(
            COMMAND "${LIB_EXE}" /nologo "/DEF:${_def}" "/OUT:${_implib}" /MACHINE:X64
            RESULT_VARIABLE _lib_rc
            OUTPUT_QUIET
            ERROR_VARIABLE _lib_err
        )
        if(_lib_rc)
            message(FATAL_ERROR "lib.exe failed for ${_def}: ${_lib_err}")
        endif()
        list(APPEND FFMPEG_IMPORT_LIBS "${_implib}")
    endforeach()
elseif(MINGW)
    get_filename_component(_MINGW_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(DLLTOOL_EXE "${_MINGW_BIN}/dlltool.exe")
    if(NOT EXISTS "${DLLTOOL_EXE}")
        message(FATAL_ERROR "Could not find dlltool.exe next to ${CMAKE_CXX_COMPILER} (MinGW).")
    endif()
    foreach(_def ${FFMPEG_DEF_FILES})
        get_filename_component(_stem "${_def}" NAME_WE)
        set(_dll "${FFMPEG_ROOT}/bin/${_stem}.dll")
        if(NOT EXISTS "${_dll}")
            message(FATAL_ERROR "Missing ${_dll} for ${_def}. Use a matching FFmpeg shared dev tree in vendor/ffmpeg.")
        endif()
        set(_implib "${FFMPEG_IMP_DIR}/lib${_stem}.dll.a")
        execute_process(
            COMMAND "${DLLTOOL_EXE}" --input-def "${_def}" --dllname "${_stem}.dll" --output-lib "${_implib}"
            RESULT_VARIABLE _dt_rc
            OUTPUT_QUIET
            ERROR_VARIABLE _dt_err
        )
        if(_dt_rc)
            message(FATAL_ERROR "dlltool failed for ${_def}: ${_dt_err}")
        endif()
        list(APPEND FFMPEG_IMPORT_LIBS "${_implib}")
    endforeach()
else()
    message(FATAL_ERROR "FFmpeg import libs: use MSVC or MinGW with vendor/ffmpeg (include, lib/*.def, bin/*.dll).")
endif()


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin)

set(STATES_SOURCES
        source/states/play-state.hpp
        source/states/menu-state.hpp
        source/states/gameover-state.hpp
        source/states/victory-state.hpp
        source/states/shader-test-state.hpp
        source/states/mesh-test-state.hpp
        source/states/transform-test-state.hpp
        source/states/pipeline-test-state.hpp
        source/states/texture-test-state.hpp
        source/states/sampler-test-state.hpp
        source/states/material-test-state.hpp
        source/states/entity-test-state.hpp
        source/states/renderer-test-state.hpp
        source/common/physics/physics-types.hpp
        source/common/physics/physics-world.hpp
        source/common/physics/physics-system.hpp
        source/common/physics/physics-world.cpp
        source/common/components/rigid-body.hpp
        source/common/components/collider.hpp
        source/common/logger.cpp
)

# For each example, we add an executable target
# Each target compiles one example source file and the common & vendor source files
# Then we link GLFW with each target
file(COPY ${PROJECT_SOURCE_DIR}/config DESTINATION ${PROJECT_SOURCE_DIR}/bin)
file(COPY ${PROJECT_SOURCE_DIR}/assets DESTINATION ${PROJECT_SOURCE_DIR}/bin)
add_executable(GAME_APPLICATION source/main.cpp ${STATES_SOURCES} ${COMMON_SOURCES} ${VENDOR_SOURCES})
target_link_libraries(GAME_APPLICATION PRIVATE glfw reactphysics3d assimp ${FFMPEG_IMPORT_LIBS})
if(WIN32)
    target_link_libraries(GAME_APPLICATION PRIVATE ws2_32 Secur32 bcrypt ole32 user32)
endif()

file(GLOB FFMPEG_DLLS "${FFMPEG_ROOT}/bin/*.dll")
foreach(_dll ${FFMPEG_DLLS})
    add_custom_command(TARGET GAME_APPLICATION POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_dll}" "$<TARGET_FILE_DIR:GAME_APPLICATION>"
    )
endforeach()