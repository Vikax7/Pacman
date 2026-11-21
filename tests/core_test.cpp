#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "Player.hpp"
#include "Board.hpp"
#include "Ghost.hpp"

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

TEST_CASE("Player moves in four directions and cannot walk through walls or bounds") {
    TempLevelFile file(
        "#####\n"
        "#P..#\n"
        "#.#.#\n"
        "#...#\n"
        "#####\n"
    );

    Board board;
    board.loadFromFile(file.path());

    Player player(board.getPlayerStart(), 3);
    CHECK(player.getPosition() == GridPos{1, 1});
    CHECK(player.getLives() == 3);
    CHECK(player.getScore() == 0);
    CHECK(player.isAlive());

    // Влево в (0, 1) — стена
    CHECK_FALSE(player.canMove(board, Direction::left));
    CHECK_FALSE(player.tryMove(board, Direction::left));
    CHECK(player.getPosition() == GridPos{1, 1});

    // Вверх в (1, 0) — стена
    CHECK_FALSE(player.canMove(board, Direction::up));
    CHECK_FALSE(player.tryMove(board, Direction::up));
    CHECK(player.getPosition() == GridPos{1, 1});

    // Вправо в (2, 1) — свободно
    CHECK(player.canMove(board, Direction::right));
    CHECK(player.tryMove(board, Direction::right));
    CHECK(player.getPosition() == GridPos{2, 1});
    CHECK(player.getCurrentDirection() == Direction::right);

    // Вниз в (2, 2) — стена
    CHECK_FALSE(player.canMove(board, Direction::down));
    CHECK_FALSE(player.tryMove(board, Direction::down));
    CHECK(player.getPosition() == GridPos{2, 1});

    // Движение влево обратно в (1, 1), затем вниз в (1, 2) — свободно
    CHECK(player.tryMove(board, Direction::left));
    CHECK(player.canMove(board, Direction::down));
    CHECK(player.tryMove(board, Direction::down));
    CHECK(player.getPosition() == GridPos{1, 2});
    CHECK(player.getCurrentDirection() == Direction::down);
}

TEST_CASE("Player tracks score, loses lives down to zero, and respawns cleanly") {
    Player player(GridPos{1, 1}, 2);

    player.addScore(10);
    player.addScore(50);
    player.addScore(-20); // некорректное/отрицательное значение не уменьшает счёт
    CHECK(player.getScore() == 60);

    player.setDesiredDirection(Direction::right);
    player.setCurrentDirection(Direction::right);
    player.setDesiredDirection(Direction::right);

    // Первая потеря жизни и возврат на старт
    player.loseLife();
    CHECK(player.getLives() == 1);
    CHECK(player.isAlive());
    player.respawn(GridPos{3, 4});
    CHECK(player.getPosition() == GridPos{3, 4});
    CHECK(player.getCurrentDirection() == Direction::none);
    CHECK(player.getDesiredDirection() == Direction::none);
    CHECK(player.getScore() == 60);

    // Вторая потеря жизни -> 0 жизней (поражение)
    player.loseLife();
    CHECK(player.getLives() == 0);
    CHECK_FALSE(player.isAlive());

    // Повторный loseLife() не уходит в отрицательные жизни
    player.loseLife();
    CHECK(player.getLives() == 0);
}





TEST_CASE("Ghost constructor validates strategy and normalizes initial state") {
    Board board;
    Player player(GridPos{1, 1}, 3);

    CHECK_THROWS_AS(
        Ghost(GhostRole::Chaser, GridPos{1, 1}, nullptr),
        std::invalid_argument
    );

    Ghost ghost_from_frightened(
        GhostRole::Chaser,
        GridPos{2, 2},
        std::make_unique<ChaserStrategy>(),
        GhostState::Frightened,
        200
    );
    CHECK(ghost_from_frightened.getState() == GhostState::Chase);
    CHECK(ghost_from_frightened.canHarmPlayer());
    CHECK_FALSE(ghost_from_frightened.isVulnerable());
    CHECK_FALSE(ghost_from_frightened.isReturning());

    Ghost ghost_from_returning(
        GhostRole::Patrol,
        GridPos{3, 3},
        std::make_unique<PatrolStrategy>(std::vector<GridPos>{{3, 3}}),
        GhostState::Returning,
        300
    );
    CHECK(ghost_from_returning.getState() == GhostState::Chase);
    CHECK(ghost_from_returning.getRewardPoints() == 300);
    (void)board;
    (void)player;
}

TEST_CASE("Ghost findNextStepBFS finds shortest path around walls and handles unreachable targets") {
    // Стена в (2, 1) перекрывает прямой путь из (1, 1) в (3, 1),
    // поэтому кратчайший путь идёт через (1, 2) -> (2, 2) -> (3, 2) -> (3, 1).
    TempLevelFile file(
        "#####\n"
        "#P#.#\n"
        "#...#\n"
        "#..##\n"
        "#####\n"
    );

    Board board;
    board.loadFromFile(file.path());

    CHECK(Ghost::findNextStepBFS(board, GridPos{1, 1}, GridPos{1, 1}) == Direction::none);
    CHECK(Ghost::findNextStepBFS(board, GridPos{1, 1}, GridPos{3, 1}) == Direction::down);

    // Клетка (3, 3) окружена стенами и недостижима из (1, 1)
    CHECK(Ghost::findNextStepBFS(board, GridPos{1, 1}, GridPos{3, 3}) == Direction::none);
}

TEST_CASE("Ghost strategies choose expected targets including patrol waypoints and aggro") {
    TempLevelFile file(
        "#######\n"
        "#P....#\n"
        "#.....#\n"
        "#.....#\n"
        "#######\n"
    );

    Board board;
    board.loadFromFile(file.path());

    Player player(GridPos{1, 1}, 3);
    player.tryMove(board, Direction::right); // игрок теперь в (2, 1), направление right

    ChaserStrategy chaser;
    CHECK(chaser.chooseTarget(board, player, GridPos{5, 3}, GridPos{5, 3}) == GridPos{2, 1});

    AmbusherStrategy ambusher(2);
    CHECK(ambusher.chooseTarget(board, player, GridPos{5, 3}, GridPos{5, 3}) == GridPos{4, 1});

    // Патрульный далеко от игрока (расстояние до (2, 1) из (5, 3) равно |5-2| + |3-1| = 5)
    PatrolStrategy patrol({GridPos{5, 3}, GridPos{2, 3}}, 2);
    // На старте ghost_pos == waypoints_[0], поэтому переключается на waypoints_[1]
    CHECK(patrol.chooseTarget(board, player, GridPos{5, 3}, GridPos{5, 3}) == GridPos{2, 3});

    // Если игрок подошёл в радиус aggro_radius == 2, патрульный переключается на позицию игрока
    CHECK(patrol.chooseTarget(board, player, GridPos{3, 2}, GridPos{5, 3}) == GridPos{2, 1});
}

TEST_CASE("Ghost state machine handles frightened timer, being eaten, returning home, and respawn") {
    TempLevelFile file(
        "#######\n"
        "#P....#\n"
        "#.....#\n"
        "#.....#\n"
        "#######\n"
    );

    Board board;
    board.loadFromFile(file.path());

    Player player(GridPos{1, 1}, 3);
    Ghost ghost(
        GhostRole::Chaser,
        GridPos{5, 2},
        std::make_unique<ChaserStrategy>(),
        GhostState::Chase,
        250
    );

    // Неуязвимого призрака съесть нельзя
    CHECK(ghost.onEatenByPlayer() == 0);
    CHECK(ghost.getState() == GhostState::Chase);

    // Активация Frightened и продление таймера через std::max
    ghost.triggerFrightened(2);
    CHECK(ghost.isVulnerable());
    CHECK_FALSE(ghost.canHarmPlayer());
    CHECK(ghost.getFrightenedTicks() == 2);

    ghost.triggerFrightened(1);
    CHECK(ghost.getFrightenedTicks() == 2);

    // Игрок съедает призрака -> переход в Returning и начисление очков
    CHECK(ghost.onEatenByPlayer() == 250);
    CHECK(ghost.isReturning());
    CHECK(ghost.getFrightenedTicks() == 0);
    CHECK_FALSE(ghost.canHarmPlayer());

    // В состоянии Returning вызов triggerFrightened не сбивает возвращение домой
    ghost.triggerFrightened(10);
    CHECK(ghost.isReturning());

    // Делаем шаги в сторону home_pos_ (5, 2) и проверяем возврат в Chase по прибытии
    for (int i = 0; i < 6 && ghost.isReturning(); ++i) {
        ghost.update(board, player);
    }
    CHECK(ghost.getPosition() == GridPos{5, 2});
    CHECK(ghost.getState() == GhostState::Chase);

    // Проверка истечения таймера Frightened через tickTimers()
    ghost.triggerFrightened(2);
    ghost.tickTimers();
    CHECK(ghost.getFrightenedTicks() == 1);
    CHECK(ghost.isVulnerable());
    ghost.tickTimers();
    CHECK(ghost.getFrightenedTicks() == 0);
    CHECK(ghost.getState() == GhostState::Chase);

    // Проверка полного сброса через respawnAtHome()
    ghost.triggerFrightened(5);
    ghost.update(board, player);
    ghost.respawnAtHome();
    CHECK(ghost.getPosition() == GridPos{5, 2});
    CHECK(ghost.getState() == GhostState::Chase);
    CHECK(ghost.getFrightenedTicks() == 0);
    CHECK(ghost.getDirection() == Direction::none);
}