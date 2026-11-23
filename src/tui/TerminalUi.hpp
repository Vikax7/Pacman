#pragma once

#include "AppController.hpp"

class TerminalUi {
public:
    TerminalUi();
    ~TerminalUi();

    TerminalUi(const TerminalUi&) = delete;
    TerminalUi& operator=(const TerminalUi&) = delete;

    void render(const AppController& app) const;
    AppCommand pollCommand() const;
    void run(AppController& app) const;
};