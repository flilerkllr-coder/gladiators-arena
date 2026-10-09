# =============================================================================
#  CMakeLists.txt — сборка симулятора гладиаторских боёв
#
#  Использование:
#      cmake -B build -DCMAKE_BUILD_TYPE=Release
#      cmake --build build -j
#
#  Если raylib не установлен в системе, положите заголовок и библиотеку в
#  third_party/raylib/{include,lib} — CMake найдёт их автоматически.
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(gladiator_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories("${CMAKE_SOURCE_DIR}/include")

set(SOURCES
    src/main.cpp
    src/gladiator.cpp
    src/arena.cpp
    src/combat_system.cpp
    src/ai_controller.cpp
    src/battle.cpp
    src/statistics.cpp
)

# -----------------------------------------------------------------------------
# Поиск Raylib: сначала через find_package, затем локальный third_party
# -----------------------------------------------------------------------------
find_package(raylib QUIET)

set(GLAD_HAVE_RAYLIB OFF)
if(raylib_FOUND)
    add_executable(gladiator_sim ${SOURCES} src/renderer.cpp)
    target_link_libraries(gladiator_sim PRIVATE raylib)
    target_compile_definitions(gladiator_sim PRIVATE GLAD_HAS_RAYLIB=1)
    set(GLAD_HAVE_RAYLIB ON)
elseif(EXISTS "${CMAKE_SOURCE_DIR}/third_party/raylib/include/raylib.h")
    message(STATUS "Raylib найден локально в third_party/")
    add_executable(gladiator_sim ${SOURCES} src/renderer.cpp)
    target_include_directories(gladiator_sim PRIVATE "${CMAKE_SOURCE_DIR}/third_party/raylib/include")
    if(WIN32)
        target_link_libraries(gladiator_sim PRIVATE
            "${CMAKE_SOURCE_DIR}/third_party/raylib/lib/libraylib.a"
            winmm.gdi32.opengl32)
    else()
        find_package(Threads REQUIRED)
        find_package(X11 REQUIRED)
        target_link_directories(gladiator_sim PRIVATE "${CMAKE_SOURCE_DIR}/third_party/raylib/lib")
        target_link_libraries(gladiator_sim PRIVATE
            raylib m GL X11 pthread dl rt asound ${X11_Xrandr_LIB} ${X11_Xcursor_LIB}
            ${X11_Xi_LIB} ${X11_Xinerama_LIB})
    endif()
    target_compile_definitions(gladiator_sim PRIVATE GLAD_HAS_RAYLIB=1)
    set(GLAD_HAVE_RAYLIB ON)
else()
    message(STATUS "Raylib не найден — собирается headless-версия (режим --simulate)")
    add_executable(gladiator_sim ${SOURCES})
endif()

if(UNIX AND NOT APPLE AND NOT GLAD_HAVE_RAYLIB)
    target_link_libraries(gladiator_sim PRIVATE pthread)
endif()

# Предупреждения для «культуры кода» (можно показать в ПЗ)
target_compile_options(gladiator_sim PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>)
