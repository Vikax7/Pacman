#pragma once

#include "Board.hpp"
#include "Player.hpp"

#include <memory>

enum class GhostState {
    Chase,
    Patrol,
    Frightened,
    Returning
};

enum class GhostRole {
    Chaser,
    Ambusher,
    Interceptor,
    Patrol
};

// Полиморфная стратегия выбора целевой клетки в лабиринте
class TargetStrategy {
public:
    virtual ~TargetStrategy() = default;

    virtual GridPos chooseTarget(
        const Board& board,
        const Player& player,
        GridPos ghost_pos,
        GridPos home_pos
    ) = 0;
};

// 1. Преследователь: цель — текущая позиция игрока
class ChaserStrategy final : public TargetStrategy {
public:
    GridPos chooseTarget(
        const Board& board,
        const Player& player,
        GridPos ghost_pos,
        GridPos home_pos
    ) override;
};

// 2. Предсказатель: цель — на несколько клеток впереди игрока по его направлению
class AmbusherStrategy final : public TargetStrategy {
private:
    int lookahead_steps_ = 4;

public:
    explicit AmbusherStrategy(int lookahead_steps = 4) noexcept
        : lookahead_steps_(lookahead_steps) {}

    GridPos chooseTarget(
        const Board& board,
        const Player& player,
        GridPos ghost_pos,
        GridPos home_pos
    ) override;
};

// 3. Перехватчик: пытается занять проход/точку на вероятном пути игрока
class InterceptorStrategy final : public TargetStrategy {
private:
    int lead_steps_ = 2;
    int flank_offset_ = 2;

public:
    InterceptorStrategy(int lead_steps = 2, int flank_offset = 2) noexcept
        : lead_steps_(lead_steps), flank_offset_(flank_offset) {}

    GridPos chooseTarget(
        const Board& board,
        const Player& player,
        GridPos ghost_pos,
        GridPos home_pos
    ) override;
};

// 4. Патрульный: ходит между контрольными точками, при близости игрока — преследует
class PatrolStrategy final : public TargetStrategy {
private:
    std::vector<GridPos> waypoints_;
    size_t current_waypoint_ = 0;
    int aggro_radius_ = 5;

public:
    PatrolStrategy(std::vector<GridPos> waypoints, int aggro_radius = 5);

    GridPos chooseTarget(
        const Board& board,
        const Player& player,
        GridPos ghost_pos,
        GridPos home_pos
    ) override;
};

class Ghost {
    private:
        GhostRole role_ = GhostRole::Chaser;
        GhostState state_ = GhostState::Chase;
        GhostState normal_state_ = GhostState::Chase;
        GridPos pos_{};
        GridPos home_pos_{};
        Direction dir_ = Direction::none;
        int frightened_ticks_ = 0;
        int reward_points_ = 200;
        std::unique_ptr<TargetStrategy> strategy_;

    public:
        Ghost(
        GhostRole role,
        GridPos start_pos,
        std::unique_ptr<TargetStrategy> strategy,
        GhostState initial_normal_state = GhostState::Chase,
        int reward_points = 200
        );

        GhostRole getRole() const noexcept { return role_; }
        GhostState getState() const noexcept { return state_; }
        GridPos getPosition() const noexcept { return pos_; }
        GridPos getHomePosition() const noexcept { return home_pos_; }
        Direction getDirection() const noexcept { return dir_; }
        int getFrightenedTicks() const noexcept { return frightened_ticks_; }
        int getRewardPoints() const noexcept { return reward_points_; }

        bool isVulnerable() const noexcept { return state_ == GhostState::Frightened; }
        bool isReturning() const noexcept { return state_ == GhostState::Returning; }
        bool canHarmPlayer() const noexcept {
            return state_ == GhostState::Chase || state_ == GhostState::Patrol;
        }

        // Переходы состояний
        void triggerFrightened(int duration_ticks) noexcept;
        void tickTimers() noexcept;
        int onEatenByPlayer() noexcept; // возвращает очки (0, если не был уязвим)
        void respawnAtHome() noexcept;

        // Выбор цели и шаг с использованием BFS по лабиринту
        GridPos computeDesiredTarget(const Board& board, const Player& player);
        static Direction findNextStepBFS(const Board& board, GridPos from, GridPos to);
        void update(const Board& board, const Player& player);
    };