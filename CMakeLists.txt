cmake_minimum_required(VERSION 3.20)
project(nexa VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Apple Silicon: force arm64
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" OR CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build for Apple Silicon")
endif()

# ── Engine selection ────────────────────────────────
option(NEXA_USE_STUB "Use stub engine instead of V8" OFF)

if(NEXA_USE_STUB)
    add_library(nexa_engine STATIC core/stub/stub_engine.cpp)
    target_include_directories(nexa_engine PUBLIC core/stub)
    target_compile_definitions(nexa_engine PUBLIC NEXA_USE_STUB_ENGINE)
else()
    # Prefer Homebrew Node (arm64), fall back to /usr/local (x86_64)
    set(NODE_INCLUDE_DIR "/opt/homebrew/include/node")
    if(NOT EXISTS "${NODE_INCLUDE_DIR}/v8.h")
        set(NODE_INCLUDE_DIR "/usr/local/include/node")
    endif()
    if(NOT EXISTS "${NODE_INCLUDE_DIR}/v8.h")
        message(FATAL_ERROR "V8 headers not found. Install Node.js or use -DNEXA_USE_STUB=ON")
    endif()

    if(EXISTS "/opt/homebrew/lib/libnode.147.dylib")
        set(LIBNODE_LIB "/opt/homebrew/lib/libnode.147.dylib")
    elseif(EXISTS "/opt/homebrew/lib/libnode.dylib")
        set(LIBNODE_LIB "/opt/homebrew/lib/libnode.dylib")
    else()
        find_library(LIBNODE_LIB NAMES node node.147 node.141
            PATHS /opt/homebrew/lib /usr/local/lib)
    endif()
    if(NOT LIBNODE_LIB)
        message(FATAL_ERROR "libnode not found. Install Node.js or use -DNEXA_USE_STUB=ON")
    endif()

    message(STATUS "V8 headers: ${NODE_INCLUDE_DIR}")
    message(STATUS "libnode:    ${LIBNODE_LIB}")

    add_library(nexa_engine STATIC core/v8/v8_engine.cpp)
    target_include_directories(nexa_engine PUBLIC core/v8 ${NODE_INCLUDE_DIR})
    target_link_libraries(nexa_engine PUBLIC ${LIBNODE_LIB})
endif()

# ── Core library ──────────────────────────────────
add_library(nexa_core STATIC
    core/nexa.cpp
    core/pool/isolate_pool.cpp
    core/pool/isolate_handle.cpp
    core/pool/process_isolate.cpp
    core/bridge/module_registry.cpp
    core/bridge/direct_buffer.cpp
)
target_include_directories(nexa_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/core/pool
    ${CMAKE_CURRENT_SOURCE_DIR}/core/bridge
    ${CMAKE_CURRENT_SOURCE_DIR}/core/ipc
)
if(NEXA_USE_STUB)
    target_compile_definitions(nexa_core PUBLIC NEXA_USE_STUB_ENGINE)
endif()
target_link_libraries(nexa_core PUBLIC nexa_engine)

# ── nexa_child (V8 only) ───────────────────────────
if(NOT NEXA_USE_STUB)
    add_executable(nexa_child core/ipc/nexa_child.cpp)
    target_include_directories(nexa_child PRIVATE ${NODE_INCLUDE_DIR} core/v8 core)
    target_link_libraries(nexa_child PRIVATE nexa_engine ${LIBNODE_LIB})

    option(NEXA_BUILD_SNAPSHOT "Build snapshot CLI" ON)
    if(NEXA_BUILD_SNAPSHOT)
        add_executable(nexa-snapshot tools/nexa_snapshot.cpp)
        target_include_directories(nexa-snapshot PRIVATE ${NODE_INCLUDE_DIR})
        target_link_libraries(nexa-snapshot PRIVATE ${LIBNODE_LIB})
    endif()
endif()

# ── Shared library ────────────────────────────────
add_library(nexa SHARED bindings/c/nexa_c_api.cpp)
target_link_libraries(nexa PRIVATE nexa_core)

# ── JNI binding ───────────────────────────────────
find_package(JNI REQUIRED)
add_library(nexa_jni SHARED bindings/jni/jni_bridge.cpp)
target_include_directories(nexa_jni PRIVATE ${JNI_INCLUDE_DIRS})
target_link_libraries(nexa_jni PRIVATE nexa_core ${LIBNODE_LIB} ${JNI_LIBRARIES})

# ── Tests ─────────────────────────────────────────
option(NEXA_BUILD_TESTS "Build tests" ON)
if(NEXA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
