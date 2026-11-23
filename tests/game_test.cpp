
#include <doctest/doctest.h>

#include "Game.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

std::string writeTempLevel(const std::string& filename, const std::string& map_content) {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / filename;
    std::ofstream out(path);
    out << map_content;
    out.close();
    return path.string();
}

} // namespace

TEST_CASE("Game throws on empty level list") {
    CHECK_THROWS_AS(Game({}), std::invalid_argument);
}

TEST_CASE("Game collects dot and updates score and remaining dots") {
    const std::string level_path = writeTempLevel(
        "pacman_doctest_dot.txt",
        "#####\n"
        "#P..#\n"
        "#G..#\n"
        "#####\n"
    );

    GameConfig config;
    config.point_score = 10;

    Game game({level_path}, config);
    const int initial_dots = game.getBoard().getNumDots();
    REQUIRE(initial_dots > 0);

    game.setPlayerDesiredDirection(Direction::right);
    game.update();

    CHECK(game.getPlayer().getScore() == 10);
    CHECK(game.getBoard().getNumDots() == initial_dots - 1);
    CHECK(game.getBoard().getCell(2, 1) == CellType::pass);

    std::filesystem::remove(level_path);
}

TEST_CASE("Game togglePause freezes and resumes update") {
    const std::string level_path = writeTempLevel(
        "pacman_doctest_pause.txt",
        "#####\n"
        "#P..#\n"
        "#...#\n"
        "#####\n"
    );

    Game game({level_path});
    game.setPlayerDesiredDirection(Direction::right);
    game.togglePause();
    CHECK(game.getStatus() == GameStatus::Paused);

    const GridPos pos_before = game.getPlayer().getPosition();
    game.update();
    CHECK(game.getPlayer().getPosition() == pos_before);

    game.togglePause();
    CHECK(game.getStatus() == GameStatus::Playing);
    game.update();
    CHECK(game.getPlayer().getPosition() != pos_before);

    std::filesystem::remove(level_path);
}

TEST_CASE("Game bonus triggers frightened state on ghosts") {
    const std::string level_path = writeTempLevel(
        "pacman_doctest_bonus.txt",
        "######\n"
        "#PoG.#\n"
        "#....#\n"
        "######\n"
    );

    GameConfig config;
    config.point_score = 50;
    config.bonus_score = 100;
    config.frightened_duration_ticks = 10;

    Game game({level_path}, config);

    game.setPlayerDesiredDirection(Direction::right);
    game.update();

    CHECK(game.getPlayer().getScore() >= 50);
    REQUIRE(!game.getGhosts().empty());
    CHECK(game.getGhosts()[0].isVulnerable());

    std::filesystem::remove(level_path);
}

TEST_CASE("Game advances levels and reaches Victory when all dots collected") {
    const std::string level1 = writeTempLevel(
        "pacman_doctest_lvl1.txt",
        "####\n"
        "#P.#\n"
        "####\n"
    );
    const std::string level2 = writeTempLevel(
        "pacman_doctest_lvl2.txt",
        "####\n"
        "#P.#\n"
        "####\n"
    );

    Game game({level1, level2});
    CHECK(game.getLevelNumber() == 1);
    CHECK(game.getStatus() == GameStatus::Playing);

    game.setPlayerDesiredDirection(Direction::right);
    game.update();
    CHECK(game.getLevelNumber() == 2);
    CHECK(game.getStatus() == GameStatus::Playing);

    game.setPlayerDesiredDirection(Direction::right);
    game.update();
    CHECK(game.getStatus() == GameStatus::Victory);

    std::filesystem::remove(level1);
    std::filesystem::remove(level2);
}

TEST_CASE("Game losing last life transitions to GameOver") {
    const std::string level_path = writeTempLevel(
        "pacman_doctest_gameover.txt",
        "#####\n"
        "#PG.#\n"
        "#...#\n"
        "#####\n"
    );

    GameConfig config;
    config.initial_lives = 1;

    Game game({level_path}, config);
    game.setPlayerDesiredDirection(Direction::right);
    game.update();

    CHECK(game.getStatus() == GameStatus::GameOver);

    std::filesystem::remove(level_path);
}