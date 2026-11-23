#include "TerminalUi.hpp"

#include <ncurses.h>

TerminalUi::TerminalUi() {
    initscr();
    if (has_colors()) {
        start_color();
        use_default_colors(); // чтобы фон оставался стандартным у терминала
        init_pair(1, COLOR_YELLOW, -1); // 1 — Пакман (жёлтый)
        init_pair(2, COLOR_RED, -1);    // 2 — призраки (красный)
    }
    cbreak();
    noecho();
    curs_set(0);
    timeout(320);
}

TerminalUi::~TerminalUi() {
    endwin();
}

void TerminalUi::render(const AppController& app) const {
    clear();

    if (app.getScreen() == AppScreen::StartMenu) {
        mvprintw(2, 4, "=== PAC-MAN ===");
        mvprintw(4, 4, "Start game (Enter)");
        mvprintw(5, 4, "Quit (q)");
        refresh();
        return;
    }

    const Game& game = app.getGame();
    const Board& board = game.getBoard();
    const GridPos player_pos = game.getPlayer().getPosition();

    mvprintw(0, 0, "Level: %d | Score: %d | Lives: %d | Dots: %zu",
        game.getLevelNumber(),
        game.getPlayer().getScore(),
        game.getPlayer().getLives(),
        board.getNumDots());

    for (size_t y = 0; y < board.getHeight(); ++y) {
        const int iy = static_cast<int>(y);
        const int row = iy + 2;

        for (size_t x = 0; x < board.getWidth(); ++x) {
            const int ix = static_cast<int>(x);
            const int col = ix;
            const GridPos cell_pos{ix, iy};

            if (cell_pos == player_pos) {
                attron(COLOR_PAIR(1) | A_BOLD);
                mvaddch(row, col, 'P');
                attroff(COLOR_PAIR(1) | A_BOLD);
                continue;
            }

            bool ghost_here = false;
            for (const Ghost& ghost : game.getGhosts()) {
                if (ghost.getPosition() == cell_pos) {
                    const char ghost_ch = ghost.isVulnerable() ? 'g' : 'G';
                    attron(COLOR_PAIR(2) | A_BOLD);
                    mvaddch(row, col, ghost_ch);
                    attroff(COLOR_PAIR(2) | A_BOLD);
                    ghost_here = true;
                    break;
                }
            }
            if (ghost_here) {
                continue;
            }

            char ch = ' ';
            switch (board.getCell(ix, iy)) {
                case CellType::wall:  ch = '#'; break;
                case CellType::point: ch = '.'; break;
                case CellType::bonus: ch = 'o'; break;
                case CellType::pass:
                default:              ch = ' '; break;
            }
            mvaddch(row, col, ch);
        }
    }
 
    
    const int hint_row = static_cast<int>(board.getHeight()) + 3;
    if (app.getScreen() == AppScreen::LevelVictory) {
        mvprintw(hint_row, 0, "VICTORY!  Continue the game (Enter) | New game (r) | Quit (q)");
    } else if (app.getScreen() == AppScreen::GameOver) {
        mvprintw(hint_row, 0, "GAME OVER!  New game (Enter / r) | Quit (q)");
    } else if (game.getStatus() == GameStatus::Paused) {
        mvprintw(hint_row, 0, "PAUSED! p - resume, q - quit");
    } else if (app.getScreen() == AppScreen::GameVictory) {
        mvprintw(hint_row, 0, "CONGRATULATIONS! You won the game! New game (Enter)");
    } else {
        mvprintw(hint_row, 0, "w/a/s/d or arrows - move, p - pause, r - restart, q - quit");
    }

    refresh();
}

AppCommand TerminalUi::pollCommand() const {
    const int key = getch();
    if (key == ERR) {
        return AppCommand::None;
    }
    switch (key) {
        case 'w':
        case 'W':
            return AppCommand::MoveUp;
        case 's':
        case 'S':
            return AppCommand::MoveDown;
        case 'a':
        case 'A':
            return AppCommand::MoveLeft;
        case 'd':
        case 'D':
            return AppCommand::MoveRight;
        case 'p':
        case 'P':
            return AppCommand::TogglePause;
        case 'r':
        case 'R':
            return AppCommand::Restart;
        case 'q':
        case 'Q':
            return AppCommand::Quit;
        case '\n':
            return AppCommand::StartOrContinue;
        default:
            return AppCommand::None;
    }
}

void TerminalUi::run(AppController& app) const {
    while (app.isRunning()) {
        render(app);
        const AppCommand cmd = pollCommand();
        if (!app.handleCommand(cmd)) {
            break;
        }
        app.tick();
    }
}