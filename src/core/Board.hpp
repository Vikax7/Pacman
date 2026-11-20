#pragma once
#include <vector>
#include <string>

enum class CellType {
    wall,
    pass,
    point,
    bonus
};

struct GridPos {
    int x = 0;
    int y = 0;

    bool operator==(const GridPos& other) const noexcept = default;
};

class Board {
    private:
        std::vector<std::vector<CellType>> grid_;
        size_t width_ = 0;
        size_t height_ = 0;
        size_t num_dot_ = 0;
        GridPos player_start_{};
        std::vector<GridPos> ghost_start;

    public:

        void loadFromFile(const std::string& filepath);

        size_t getWidth() const noexcept { return width_; }
        size_t getHeight() const noexcept { return height_; }
        size_t getNumDots() const noexcept { return num_dots_; }

        GridPos getPlayerStart() const noexcept { return player_start_; }
        const std::vector<GridPos>& getGhostStart() const noexcept { return ghost_start_; }

        bool isInside(int x, int y) const noexcept;
        bool isWall(int x, int y) const;
        
        CellType getCell(int x, int y) const;
        void setCell(int x, int y, CellType type);
        void decrementDots();
};