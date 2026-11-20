#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "Board.hpp"

#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

class TempLevelFile {
public:
    explicit TempLevelFile(const std::string& content)
        : path_("temp_test_level.txt") {
        std::ofstream out(path_);
        out << content;
    }

    ~TempLevelFile() {
        std::remove(path_.c_str());
    }

    const std::string& path() const noexcept { return path_; }

private:
    std::string path_;
};

TEST_CASE("Board loads valid map, counts dots and stores start positions") {
    TempLevelFile file(
        "#######\n"
        "#P.o.G#\n"
        "#.#.#.#\n"
        "#..G..#\n"
        "#######\n"
    );

    Board board;
    board.loadFromFile(file.path());

    CHECK(board.getWidth() == 7);
    CHECK(board.getHeight() == 5);
    CHECK(board.getNumDots() == 10);
    CHECK(board.getPlayerStart() == GridPos{1, 1});

    REQUIRE(board.getGhostStart().size() == 2);
    CHECK(board.getGhostStart()[0] == GridPos{5, 1});
    CHECK(board.getGhostStart()[1] == GridPos{3, 3});

    CHECK(board.getCell(0, 0) == CellType::wall);
    CHECK(board.getCell(1, 1) == CellType::pass);
    CHECK(board.getCell(2, 1) == CellType::point);
    CHECK(board.getCell(3, 1) == CellType::bonus);
}

TEST_CASE("Board checks bounds, walls and dot consumption") {
    TempLevelFile file(
        "####\n"
        "#P.#\n"
        "####\n"
    );

    Board board;
    board.loadFromFile(file.path());

    CHECK(board.isInside(0, 0));
    CHECK(board.isInside(3, 2));
    CHECK_FALSE(board.isInside(-1, 0));
    CHECK_FALSE(board.isInside(4, 0));
    CHECK_FALSE(board.isInside(0, 3));

    CHECK(board.isWall(0, 0));
    CHECK(board.isWall(-1, 1));
    CHECK_FALSE(board.isWall(1, 1));
    CHECK_FALSE(board.isWall(2, 1));

    CHECK(board.getNumDots() == 1);
    board.setCell(2, 1, CellType::pass);
    board.decrementDots();
    CHECK(board.getCell(2, 1) == CellType::pass);
    CHECK(board.getNumDots() == 0);
    // Повторный вызов decrementDots не уходит в отрицательные значения
    board.decrementDots();
    CHECK(board.getNumDots() == 0);

    CHECK_THROWS_AS(board.getCell(-1, 0), std::out_of_range);
    CHECK_THROWS_AS(board.setCell(10, 10, CellType::pass), std::out_of_range);
}

TEST_CASE("Board rejects invalid maps with clear exceptions") {
    Board board;

    // Несуществующий файл
    CHECK_THROWS_AS(board.loadFromFile("non_existent_level_12345.txt"), std::runtime_error);

    // Непрямоугольная карта
    {
        TempLevelFile non_rect(
            "#####\n"
            "#P.#\n"
            "#####\n"
        );
        CHECK_THROWS_AS(board.loadFromFile(non_rect.path()), std::runtime_error);
    }

    // Недопустимый символ
    {
        TempLevelFile bad_char(
            "#####\n"
            "#P?##\n"
            "#####\n"
        );
        CHECK_THROWS_AS(board.loadFromFile(bad_char.path()), std::runtime_error);
    }

    // Отсутствует игрок (P)
    {
        TempLevelFile no_player(
            "#####\n"
            "#...#\n"
            "#####\n"
        );
        CHECK_THROWS_AS(board.loadFromFile(no_player.path()), std::runtime_error);
    }

    // Более одного игрока (P)
    {
        TempLevelFile two_players(
            "#####\n"
            "#P.P#\n"
            "#####\n"
        );
        CHECK_THROWS_AS(board.loadFromFile(two_players.path()), std::runtime_error);
    }
}