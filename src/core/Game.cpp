#include "Game.hpp"

std::unique_ptr<TargetStrategy> createGhostStrategy(
    GhostRole role,
    const Board& board,
    const GameConfig& config
) {
    switch (role) {
        case GhostRole::Chaser:
            return std::make_unique<ChaserStrategy>();
        case GhostRole::Ambusher:
            return std::make_unique<AmbusherStrategy>(config.ambusher_lookahead);
        case GhostRole::Interceptor:
            return std::make_unique<InterceptorStrategy>(
                config.interceptor_lead_steps,
                config.interceptor_flank_offset
            );
        case GhostRole::Patrol: {
            const int board_width = static_cast<int>(board.getWidth());
            const int board_height = static_cast<int>(board.getHeight());
            std::vector<GridPos> patrol_points;

            for (int y = 0; y < board_height; ++y) {
                for (int x = 0; x < board_width; ++x) {
                    if (board.getCell(x, y) == CellType::bonus) {
                        patrol_points.push_back(GridPos{x, y});
                    }
                }
            }

            if (patrol_points.empty()) {
                const int right_edge_x = std::max(1, board_width - 2);
                const int bottom_edge_y = std::max(1, board_height - 2);

                patrol_points = {
                    GridPos{1, 1},
                    GridPos{right_edge_x, 1},
                    GridPos{right_edge_x, bottom_edge_y},
                    GridPos{1, bottom_edge_y}
                };
            }
            return std::make_unique<PatrolStrategy>(
            std::move(patrol_points),
            config.patrol_aggro_radius
            );

        }
        default:
            return std::make_unique<ChaserStrategy>();
    }
}

GhostRole selectGhostRole(size_t ghost_index) noexcept {
    constexpr GhostRole available_roles[4] = {
        GhostRole::Chaser,
        GhostRole::Ambusher,
        GhostRole::Interceptor,
        GhostRole::Patrol
    };
    return available_roles[ghost_index % 4];
}

GhostState getDefaultStateForRole(GhostRole role) noexcept {
    return (role == GhostRole::Patrol) ? GhostState::Patrol : GhostState::Chase;
}

Game::Game(std::vector<std::string> level_paths, GameConfig config)
    : config_(config),
      player_(GridPos{0, 0}, std::max(0, config.initial_lives)),
      level_files_(std::move(level_paths)) {
    if (level_files_.empty()) {
        throw std::invalid_argument("Список файлов уровней не должен быть пустым");
    }
    startNewGame();
}

void Game::startNewGame() {
    current_level_index_ = 0;
    player_ = Player(GridPos{0, 0}, std::max(0, config_.initial_lives));
    loadLevelByIndex(current_level_index_);
    status_ = (board_.getNumDots() == 0) ? GameStatus::Victory : GameStatus::Playing;
}

void Game::advanceLevelOrRestart() {
    if (current_level_index_ + 1 < level_files_.size()) {
        ++current_level_index_;
        loadLevelByIndex(current_level_index_);
        status_ = GameStatus::Playing;
    } else {
        startNewGame();
    }
}

void Game::loadLevelByIndex(size_t level_index) {
    if (level_index >= level_files_.size()) {
        throw std::out_of_range("Индекс уровня выходит за пределы списка уровней");
    }
    board_.loadFromFile(level_files_[level_index]);
    const int current_score = player_.getScore();
    const int remaining_lives = config_.initial_lives;
    player_ = Player(board_.getPlayerStart(), remaining_lives);
    player_.addScore(current_score);
    spawnGhosts();
}

void Game::spawnGhosts() {
    ghosts_.clear();
    const std::vector<GridPos>& ghost_spawn_positions = board_.getGhostStart();
    ghosts_.reserve(ghost_spawn_positions.size());

    for (size_t i = 0; i < ghost_spawn_positions.size(); ++i) {
        const GhostRole role = selectGhostRole(i);
        const GhostState default_state = getDefaultStateForRole(role);
        auto strategy = createGhostStrategy(role, board_, config_);
        ghosts_.emplace_back(
            role,
            ghost_spawn_positions[i],
            std::move(strategy),
            default_state,
            config_.ghost_reward_points
        );
    }
}

void Game::setPlayerDesiredDirection(Direction direction) noexcept {
    player_.setDesiredDirection(direction);
}

void Game::togglePause() noexcept {
    if (status_ == GameStatus::Playing) {
        status_ = GameStatus::Paused;
    } else if (status_ == GameStatus::Paused) {
        status_ = GameStatus::Playing;
    }
}

void Game::collectItem(GridPos player_position) {
    if (!board_.isInside(player_position.x, player_position.y)) {
        return;
    }
    const CellType current_cell = board_.getCell(player_position.x, player_position.y);
    if (current_cell == CellType::point) {
        board_.setCell(player_position.x, player_position.y, CellType::pass);
        board_.decrementDots();
        player_.addScore(config_.point_score);
    } else if (current_cell == CellType::bonus) {
        board_.setCell(player_position.x, player_position.y, CellType::pass);
        board_.decrementDots();
        player_.addScore(config_.bonus_score);
        for (Ghost& ghost : ghosts_) {
            ghost.triggerFrightened(config_.frightened_duration_ticks);
        }
    }
}

void Game::respawnEntitiesAfterDeath() {
    player_.respawn(board_.getPlayerStart());
    for (Ghost& ghost : ghosts_) {
        ghost.respawnAtHome();

    }
}

void Game::checkPlayerGhostCollisions() {
    const GridPos player_position = player_.getPosition();
    for (Ghost& ghost : ghosts_) {
        if (ghost.getPosition() != player_position) {
            continue;
        }
        if (ghost.isVulnerable()) {
            const int earned_points = ghost.onEatenByPlayer();
            player_.addScore(earned_points);
        } else if (ghost.canHarmPlayer()) {
            player_.loseLife();
            if (player_.isAlive()) {
                respawnEntitiesAfterDeath();
            } else {
                status_ = GameStatus::GameOver;
            }
            return;
        }
    }
}

void Game::update() {
    if (status_ != GameStatus::Playing) {
        return;
    }

    const Direction desired_direction = player_.getDesiredDirection();
    const Direction current_direction = player_.getCurrentDirection();
    if (desired_direction != Direction::none && player_.tryMove(board_, desired_direction)) {
        // Игрок повернул в желаемом направлении
    } else if (current_direction != Direction::none) {
        player_.tryMove(board_, current_direction);
    }

    collectItem(player_.getPosition());

    if (board_.getNumDots() == 0) {
        status_ = GameStatus::Victory;
        return;
    }

    checkPlayerGhostCollisions();
    if (status_ != GameStatus::Playing) {
        return;
    }

    for (Ghost& ghost : ghosts_) {
        ghost.update(board_, player_);
    }

    checkPlayerGhostCollisions();
    if (status_ != GameStatus::Playing) {
        return;
    }

    
}