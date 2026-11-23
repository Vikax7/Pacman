cmake_minimum_required(VERSION 3.20)
project(pacman_game CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

include(FetchContent)

# 1. Игровая модель / ядро (не зависит от интерфейса)
add_library(pacman_core
    src/core/Board.hpp
    src/core/Board.cpp
    src/core/Player.hpp
    src/core/Player.cpp
    src/core/Ghost.hpp
    src/core/Ghost.cpp
    src/core/Game.hpp
    src/core/Game.cpp
)

target_include_directories(pacman_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

# 2. Слой графического интерфейса
add_library(pacman_ui
    src/gui/placeholder.cpp
)

target_include_directories(pacman_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
)

target_link_libraries(pacman_ui
    PUBLIC
        pacman_core
)

# 3. Исполняемое приложение
add_executable(pacman_app
    src/app/main.cpp
)

target_link_libraries(pacman_app
    PRIVATE
        pacman_core
        pacman_ui
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(TARGET pacman_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:pacman_app>/assets"
    )
endif()

# 4. Автоматические тесты (только ядро)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
)
FetchContent_MakeAvailable(doctest)

add_executable(pacman_tests
    tests/core_test.cpp
)

target_link_libraries(pacman_tests
    PRIVATE
        pacman_core
        doctest::doctest
)

enable_testing()
add_test(NAME CoreTests COMMAND pacman_tests)