#pragma once

#include "Game.hpp"

enum class AppScreen {
    StartMenu,
    Playing,
    LevelVictory,
    GameOver,
    GameVictory
};

enum class AppCommand {
    None,
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    StartOrContinue,
    TogglePause,
    Restart,
    Quit
};

class AppController {
public:
    explicit AppController(std::vector<std::string> level_paths, GameConfig config = {});

    bool handleCommand(AppCommand cmd);
    void tick();

    bool isRunning() const noexcept { return is_running_; }
    AppScreen getScreen() const noexcept {return screen_; }
    const Game& getGame() const noexcept { return game_; }

private:
    Game game_;
    AppScreen screen_ = AppScreen::StartMenu;
    bool is_running_ = true;
};