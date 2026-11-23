#pragma once

#include "Board.hpp"
#include "Ghost.hpp"
#include "Player.hpp"

enum class GameStatus {
    Playing,
    Paused,
    Victory,
    GameOver
};

struct GameConfig {
    int initial_lives = 3;
    int point_score = 10;
    int bonus_score = 50;
    int frightened_duration_ticks = 20;
    int ghost_reward_points = 200;
    int patrol_aggro_radius = 5;
    int ambusher_lookahead = 4;
    int interceptor_lead_steps = 2;
    int interceptor_flank_offset = 2;
};

class Game {
    private:
        GameConfig config_{};
        Board board_{};
        Player player_{GridPos{0, 0}, 3};
        std::vector<Ghost> ghosts_{};
        std::vector<std::string> level_files_{};
        size_t current_level_index_ = 0;
        GameStatus status_ = GameStatus::Playing;

        void loadLevelByIndex(size_t level_index);
        void spawnGhosts();
        void collectItem(GridPos player_position);
        void checkPlayerGhostCollisions();
        void respawnEntitiesAfterDeath();

    public:
        explicit Game(std::vector<std::string> level_files_, GameConfig config = {});

        void startNewGame();

        void setPlayerDesiredDirection(Direction direction) noexcept;
        void togglePause() noexcept;

        void update();

        GameStatus getStatus() const noexcept { return status_; }
        int getLevelNumber() const noexcept { return static_cast<int>(current_level_index_) + 1; }
        const GameConfig& getConfig() const noexcept { return config_; }
        const Board& getBoard() const noexcept { return board_; }
        const Player& getPlayer() const noexcept { return player_; }
        const std::vector<Ghost>& getGhosts() const noexcept { return ghosts_; }
};