#include "Ghost.hpp"

#include <queue>

Direction kFourDirs[4] = {
    Direction::up,
    Direction::down,
    Direction::left,
    Direction::right
};

GridPos clampToPassable(const Board& board, GridPos candidate, GridPos fallback) {
    if (board.isInside(candidate.x, candidate.y) && !board.isWall(candidate.x, candidate.y)) {
        return candidate;
    }

    // Ищем ближайшую проходимую клетку вокруг кандидата
    int best_dist = 1'000'000;
    GridPos best_pos = fallback;
    const int w = static_cast<int>(board.getWidth());
    const int h = static_cast<int>(board.getHeight());

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (!board.isWall(x, y)) {
                const int d = std::abs(x - candidate.x) + std::abs(y - candidate.y);
                if (d < best_dist) {
                    best_dist = d;
                    best_pos = GridPos{x, y};
                }
            }
        }
    }
    return best_pos;
};

GridPos ChaserStrategy::chooseTarget(
    const Board& board,
    const Player& player,
    GridPos /*ghost_pos*/,
    GridPos /*home_pos*/
) {
    return clampToPassable(board, player.getPosition(), player.getPosition());
}

GridPos AmbusherStrategy::chooseTarget(
    const Board& board,
    const Player& player,
    GridPos /*ghost_pos*/,
    GridPos /*home_pos*/
) {
    const GridPos p_pos = player.getPosition();
    const GridPos delta = directionDelta(player.getCurrentDirection());
    const GridPos ahead{
        p_pos.x + delta.x * lookahead_steps_,
        p_pos.y + delta.y * lookahead_steps_
    };
    return clampToPassable(board, ahead, p_pos);
}

GridPos InterceptorStrategy::chooseTarget(
    const Board& board,
    const Player& player,
    GridPos ghost_pos,
    GridPos /*home_pos*/
) {
    const GridPos p_pos = player.getPosition();
    const GridPos delta = directionDelta(player.getCurrentDirection());

    // Точка впереди игрока + боковое смещение для выхода на перекрёсток/проход
    const GridPos perp{-delta.y, delta.x};
    const int sign = ((ghost_pos.x + ghost_pos.y) % 2 == 0) ? 1 : -1;

    const GridPos intercept{
        p_pos.x + delta.x * lead_steps_ + perp.x * flank_offset_ * sign,
        p_pos.y + delta.y * lead_steps_ + perp.y * flank_offset_ * sign
    };
    return clampToPassable(board, intercept, p_pos);
}

PatrolStrategy::PatrolStrategy(std::vector<GridPos> waypoints, int aggro_radius)
    : waypoints_(std::move(waypoints)), aggro_radius_(std::max(0, aggro_radius)) {}

GridPos PatrolStrategy::chooseTarget(
    const Board& board,
    const Player& player,
    GridPos ghost_pos,
    GridPos home_pos
) {
    const GridPos p_pos = player.getPosition();
    const int manhattan = std::abs(p_pos.x - ghost_pos.x) + std::abs(p_pos.y - ghost_pos.y);
    if (manhattan <= aggro_radius_) {
        return clampToPassable(board, p_pos, home_pos);
    }

    if (waypoints_.empty()) {
        return clampToPassable(board, home_pos, ghost_pos);
    }

    GridPos target = waypoints_[current_waypoint_];
    if (ghost_pos == target) {
        current_waypoint_ = (current_waypoint_ + 1) % waypoints_.size();
        target = waypoints_[current_waypoint_];
    }
    return clampToPassable(board, target, home_pos);
}

Ghost::Ghost (
    GhostRole role,
    GridPos start_pos,
    std::unique_ptr<TargetStrategy> strategy,
    GhostState initial_normal_state,
    int reward_points
)
    : role_(role),
      state_(initial_normal_state == GhostState::Frightened || initial_normal_state == GhostState::Returning
                 ? GhostState::Chase
                 : initial_normal_state),
      normal_state_(initial_normal_state == GhostState::Frightened || initial_normal_state == GhostState::Returning
                        ? GhostState::Chase
                        : initial_normal_state),
      pos_(start_pos),
      home_pos_(start_pos),
      reward_points_(std::max(0, reward_points)),
      strategy_(std::move(strategy)) {
    if (!strategy_) {
        throw std::invalid_argument("Стратегия призрака не должна быть пустой.");
    }
      }

void Ghost::triggerFrightened(int duration_ticks) noexcept {
    if (duration_ticks <= 0) {
        return;
    }
    // В состоянии Returning призрак возвращается домой и не переходит в Frightened
    if (state_ == GhostState::Returning) {
        return;
    }
    state_ = GhostState::Frightened;
    frightened_ticks_ = std::max(frightened_ticks_, duration_ticks);
}

void Ghost::tickTimers() noexcept {
    if (state_ == GhostState::Frightened && frightened_ticks_ > 0) {
        --frightened_ticks_;
        if (frightened_ticks_ == 0) {
            state_ = normal_state_;
        }
    }
}

int Ghost::onEatenByPlayer() noexcept {
    if (state_ != GhostState::Frightened) {
        return 0;
    }
    state_ = GhostState::Returning;
    frightened_ticks_ = 0;
    return reward_points_;
}

void Ghost::respawnAtHome() noexcept {
    pos_ = home_pos_;
    state_ = normal_state_;
    frightened_ticks_ = 0;
    dir_ = Direction::none;
}

GridPos Ghost::computeDesiredTarget(const Board& board, const Player& player) {
    if (state_ == GhostState::Returning) {
        return home_pos_;
    }

    if (state_ == GhostState::Frightened) {
        // Убегаем в клетку, максимально удалённую от игрока по манхэттенскому расстоянию
        const GridPos p = player.getPosition();
        const int w = static_cast<int>(board.getWidth());
        const int h = static_cast<int>(board.getHeight());
        int best_dist = -1;
        GridPos best_cell = pos_;

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (!board.isWall(x, y)) {
                    const int d = std::abs(x - p.x) + std::abs(y - p.y);
                    if (d > best_dist) {
                        best_dist = d;
                        best_cell = GridPos{x, y};
                    }
                }
            }
        }
        return best_cell;
    }

    return strategy_->chooseTarget(board, player, pos_, home_pos_);
}

Direction Ghost::findNextStepBFS(const Board& board, GridPos from, GridPos to) {
    if (!board.isInside(from.x, from.y) || !board.isInside(to.x, to.y)) {
        return Direction::none;
    }
    if (from == to || board.isWall(from.x, from.y) || board.isWall(to.x, to.y)) {
        return Direction::none;
    }

    const int w = static_cast<int>(board.getWidth());
    const int h = static_cast<int>(board.getHeight());

    std::vector<std::vector<bool>> visited(
        static_cast<size_t>(h),
        std::vector<bool>(static_cast<size_t>(w), false)
    );
    std::vector<std::vector<Direction>> first_dir(
        static_cast<size_t>(h),
        std::vector<Direction>(static_cast<size_t>(w), Direction::none)
    );
    std::queue<GridPos> q;

    visited[static_cast<size_t>(from.y)][static_cast<size_t>(from.x)] = true;
    q.push(from);

    while (!q.empty()) {
        const GridPos cur = q.front();
        q.pop();

        if (cur == to) {
            return first_dir[static_cast<size_t>(to.y)][static_cast<size_t>(to.x)];
        }

        const Direction entry_dir = (cur == from)
                                        ? Direction::none
                                        : first_dir[static_cast<size_t>(cur.y)][static_cast<size_t>(cur.x)];

        for (Direction d : kFourDirs) {
            const GridPos delta = directionDelta(d);
            const int nx = cur.x + delta.x;
            const int ny = cur.y + delta.y;

            if (!board.isInside(nx, ny) || board.isWall(nx, ny)) {
                continue;
            }

            const size_t snx = static_cast<size_t>(nx);
            const size_t sny = static_cast<size_t>(ny);
            if (!visited[sny][snx]) {
                visited[sny][snx] = true;
                first_dir[sny][snx] = (cur == from) ? d : entry_dir;
                q.push(GridPos{nx, ny});
            }
        }
    }

    return Direction::none;
}

void Ghost::update(const Board& board, const Player& player) {
    if (state_ == GhostState::Frightened) {
        tickTimers();
    }

    const GridPos target = computeDesiredTarget(board, player);
    const Direction step = findNextStepBFS(board, pos_, target);

    if (step != Direction::none) {
        const GridPos delta = directionDelta(step);
        const int nx = pos_.x + delta.x;
        const int ny = pos_.y + delta.y;
        if (board.isInside(nx, ny) && !board.isWall(nx, ny)) {
            pos_ = GridPos{nx, ny};
            dir_ = step;
        }
    } else {
        dir_ = Direction::none;
    }

    if (state_ == GhostState::Returning && pos_ == home_pos_) {
        state_ = normal_state_;
        dir_ = Direction::none;
    }
}