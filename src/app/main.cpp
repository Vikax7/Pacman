#include "AppController.hpp"
#include "TerminalUi.hpp"

#include <iostream>

std::vector<std::string> getLevelPaths(int argc, char* argv[]) {
    std::vector<std::string> level_paths;
    for (int i = 1; i < argc; ++i) {
        level_paths.emplace_back(argv[i]);
    }
    if (level_paths.empty()) {
        level_paths = {
            "assets/level1.txt",
            "assets/level2.txt",
            "assets/level3.txt"
        };
    }
    return level_paths;
}

int main(int argc, char* argv[]) {
    try {
        AppController app(getLevelPaths(argc, argv));
        TerminalUi ui;
        ui.run(app);
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}