#include "AppController.hpp"

AppController::AppController(std::vector<std::string> level_paths, GameConfig config)
    : game_(std::move(level_paths), config) {}

bool AppController::handleCommand(AppCommand cmd) {
    switch (cmd) {
        case AppCommand::MoveUp:
            if (screen_ == AppScreen::Playing) {
                game_.setPlayerDesiredDirection(Direction::up);
            }
            return true;
        case AppCommand::MoveDown:
            if (screen_ == AppScreen::Playing) {
                game_.setPlayerDesiredDirection(Direction::down);
            }
            return true;
        case AppCommand::MoveLeft:
            if (screen_ == AppScreen::Playing) {
                game_.setPlayerDesiredDirection(Direction::left);
            }
            return true;
        case AppCommand::MoveRight:
            if (screen_ == AppScreen::Playing) {
                game_.setPlayerDesiredDirection(Direction::right);
            }
            return true;
        case AppCommand::StartOrContinue:
            if (screen_ == AppScreen::StartMenu) {
                game_.startNewGame();
                screen_ = AppScreen::Playing;
            } else if (screen_ == AppScreen::LevelVictory) {
                game_.advanceLevelOrRestart();
                screen_ = AppScreen::Playing;
            } else if (screen_ == AppScreen::GameOver) {
                game_.startNewGame();
                screen_ = AppScreen::Playing;
            } else if (screen_ == AppScreen::GameVictory) {
                game_.startNewGame();
                screen_ = AppScreen::Playing;
            }
            return true;
        case AppCommand::TogglePause:
            if (screen_ == AppScreen::Playing) {
                game_.togglePause();
            }
            return true;
        case AppCommand::Restart:
            game_.startNewGame();
            screen_ = AppScreen::Playing;
            return true;
        case AppCommand::Quit:
            is_running_ = false;
            return false;
        case AppCommand::None:
        default:
            return true;
    }
}

void AppController::tick() {
    if (screen_ != AppScreen::Playing) {
        return;
    }

    game_.update();

    if (game_.getStatus() == GameStatus::GameOver) {
        screen_ = AppScreen::GameOver;
    } else if (game_.getStatus() == GameStatus::Victory) {
        if (game_.getCurrentLevelIndex() + 1 < game_.getLevelFiles().size()) {
            screen_ = AppScreen::LevelVictory;
        } else {
            screen_ = AppScreen::GameVictory;
        }
    }
}