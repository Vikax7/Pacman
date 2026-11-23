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
find_package(Curses REQUIRED)

# 1. Игровая модель / ядро (не зависит от приложения и интерфейса)
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

# 2. Слой приложения (зависит от ядра)
add_library(pacman_app
    src/app/AppController.hpp
    src/app/AppController.cpp
)

target_include_directories(pacman_app
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app
)

target_link_libraries(pacman_app
    PUBLIC
        pacman_core
)

# 3. Слой интерфейса (зависит от приложения и ядра)
add_library(pacman_ui
    src/tui/TerminalUi.hpp
    src/tui/TerminalUi.cpp
)

target_include_directories(pacman_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tui
    PRIVATE
        ${CURSES_INCLUDE_DIRS}
)

target_link_libraries(pacman_ui
    PUBLIC
        pacman_app
    PRIVATE
        ${CURSES_LIBRARIES}
)

# 4. Исполняемое приложение
add_executable(pacman_app_exe
    src/app/main.cpp
)

target_link_libraries(pacman_app_exe
    PRIVATE
        pacman_ui
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(TARGET pacman_app_exe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:pacman_app_exe>/assets"
    )
endif()

# 5. Автоматические тесты (только ядро)
find_package(doctest CONFIG QUIET)
if(NOT doctest_FOUND)
    FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG        v2.4.11
    )
    FetchContent_MakeAvailable(doctest)
endif()

add_executable(pacman_tests
    tests/core_test.cpp
    tests/game_test.cpp
)

target_include_directories(pacman_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
if(TARGET doctest::doctest)
    target_link_libraries(pacman_tests
        PRIVATE
            pacman_core
            doctest::doctest
    )
else()
    target_link_libraries(pacman_tests
        PRIVATE
            pacman_core
    )
endif()

enable_testing()
add_test(NAME CoreTests COMMAND pacman_tests)