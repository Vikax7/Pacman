#include "Player.hpp"

GridPos directionDelta(Direction dir) noexcept {
    switch (dir) {
        case Direction::up:
            return GridPos{0, -1};
        case Direction::down:
            return GridPos{0, 1};
        case Direction::left:
            return GridPos{-1, 0};
        case Direction::right:
            return GridPos{1, 0};
        case Direction::none:
        default:
            return GridPos{0, 0};
    }
}

Player::Player(GridPos start_pos, int initial_lives) noexcept
    : pos_(start_pos), lives_(std::max(0, initial_lives)) {}

void Player::addScore(int points) noexcept {
    if (points > 0) {
        score_ += points;
    }
}

void Player::loseLife() noexcept {
    if (lives_ > 0) {
        --lives_;
    }
}

bool Player::canMove(const Board& board, Direction dir) const {
    if (dir == Direction::none) {
        return false;
    }
    const GridPos delta = directionDelta(dir);
    const int next_x = pos_.x + delta.x;
    const int next_y = pos_.y + delta.y;

    return board.isInside(next_x, next_y) && !board.isWall(next_x, next_y);
}

bool Player::tryMove(const Board& board, Direction dir) {
    if (!canMove(board, dir)) {
        return false;
    }
    const GridPos delta = directionDelta(dir);
    pos_.x += delta.x;
    pos_.y += delta.y;
    current_dir_ = dir;
    return true;
}

void Player::respawn(GridPos start_pos) noexcept {
    pos_ = start_pos;
    current_dir_ = Direction::none;
    desired_dir_ = Direction::none;
}