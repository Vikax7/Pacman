#pragma once

#include "Board.hpp"

enum class Direction {
    none,
    up,
    down,
    left,
    right
};

GridPos directionDelta(Direction dir) noexcept;

class Player {
    private:
        GridPos pos_{};
        Direction current_dir_ = Direction::none;
        Direction desired_dir_ = Direction::none;
        int lives_ = 3;
        int score_ = 0;

    public:
        explicit Player(GridPos start_pos, int initial_lives = 3) noexcept;

        GridPos getPosition() const noexcept { return pos_; }
        int getX() const noexcept { return pos_.x; }
        int getY() const noexcept { return pos_.y; }

        Direction getCurrentDirection() const noexcept { return current_dir_; }
        Direction getDesiredDirection() const noexcept { return desired_dir_; }
        int getLives() const noexcept { return lives_; }
        int getScore() const noexcept { return score_; }
        bool isAlive() const noexcept { return lives_ > 0; }

        void setDesiredDirection(Direction dir) noexcept { desired_dir_ = dir; }
        void setCurrentDirection(Direction dir) noexcept { current_dir_ = dir; }

        void addScore(int points) noexcept;
        void loseLife() noexcept;

        //Проверяет, можно ли сделать шаг в указанном направлении (не стена и внутри поля)
        bool canMove(const Board& board, Direction dir) const;

        //Пытается сдвинуть игрока на 1 клетку в указанном направлении.
        //Возвращает true, если перемещение выполнено; false, если путь заблокирован стеной
        bool tryMove(const Board& board, Direction dir);

        //Восстанавливает позицию на стартовую после потери жизни (очки и собранные точки сохраняются)
        void respawn(GridPos start_pos) noexcept;

        
};